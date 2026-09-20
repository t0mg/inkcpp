/* Copyright (c) 2024 Julian Benda
 *
 * This file is part of inkCPP which is released under MIT license.
 * See file LICENSE.txt or go to
 * https://github.com/JBenda/inkcpp for full license details.
 */
#include "globals_impl.h"
#include "story_impl.h"
#include "runner_impl.h"
#include "snapshot_impl.h"
#include "system.h"
#include "types.h"
#include "value.h"
#include <new>

namespace ink::runtime::internal
{
globals_impl::globals_impl(const story_impl* story)
    : _num_containers(story->num_containers())
    , _turn_cnt{0}
    , _visit_counts(visit_count(), visit_count_null_value())
    , _owner(story)
    , _runners_start(nullptr)
    , _lists(story->list_meta())
    , _globals_initialized(false)
{
	_visit_counts.resize(_num_containers);
	if (_lists) {
		// initialize static lists
		_lists.init_static_list_flags(_owner->lists(), _variables);
	}
}

void globals_impl::visit(uint32_t container_id)
{
	_visit_counts.set(container_id, {_visit_counts[container_id].visits + 1, 0});
}

uint32_t globals_impl::visits(uint32_t container_id) const
{
	return _visit_counts[container_id].visits;
}

uint32_t globals_impl::turns() const { return _turn_cnt; }

void globals_impl::turn()
{
	++_turn_cnt;
	for (size_t i = 0; i < _visit_counts.capacity(); ++i) {
		visit_count visits = _visit_counts[i];
		if (visits.turns != -1) {
			visits.turns += 1;
			_visit_counts.set(i, visits);
		}
	}
}

uint32_t globals_impl::turns(uint32_t container_id) const
{
	return _visit_counts[container_id].turns;
}

void globals_impl::add_runner(const runner_impl* runner)
{
	// cache start of list
	auto first = _runners_start;

	// create new entry as start, linked to the previous start
	_runners_start = new runner_entry{runner, first};
}

void globals_impl::remove_runner(const runner_impl* runner)
{
	// iterate linked list
	runner_entry* prev = nullptr;
	auto          iter = _runners_start;
	while (iter != nullptr) {
		if (iter->object == runner) {
			// Fixup next pointer
			if (prev != nullptr)
				prev->next = iter->next;
			else
				_runners_start = iter->next;

			// delete
			delete iter;
			return;
		}

		// move on to next entry
		prev = iter;
		iter = iter->next;
	}
}

void globals_impl::set_variable(hash_t name, const value& val)
{
	ink::optional<value> old_var   = ink::nullopt;
	value*               p_old_var = get_variable(name);
	if (p_old_var != nullptr) {
		old_var = *p_old_var;
	}

	_variables.set(name, val);

	for (auto& callback : _callbacks) {
		if (callback.name == name) {
			if (old_var.has_value()) {
				callback.operation->call(
				    val.to_interface_value(lists()), {old_var->to_interface_value(lists())}
				);
			} else {
				callback.operation->call(val.to_interface_value(lists()), ink::nullopt);
			}
		}
	}
}

const value* globals_impl::get_variable(hash_t name) const { return _variables.get(name); }

value* globals_impl::get_variable(hash_t name) { return _variables.get(name); }

optional<ink::runtime::value> globals_impl::get_var(hash_t name) const
{
	auto* var = get_variable(name);
	if (! var) {
		return nullopt;
	}
	return {var->to_interface_value(_lists)};
}

bool globals_impl::set_var(hash_t name, const ink::runtime::value& val)
{
	auto* var = get_variable(name);
	if (! var) {
		return false;
	}
	ink::runtime::value old_val = var->to_interface_value(lists());

	bool ret = false;
	if (val.type == ink::runtime::value::Type::String) {
		if (! (var->type() == value_type::none || var->type() == value_type::string)) {
			return false;
		}
		size_t size = 0;
		char*  ptr;
		for (const char* i = val.get<runtime::value::Type::String>(); *i; ++i) {
			++size;
		}
		char* new_string = strings().create(size + 1);
		strings().mark_used(new_string);
		ptr = new_string;
		for (const char* i = val.get<runtime::value::Type::String>(); *i; ++i) {
			*ptr++ = *i;
		}
		*ptr = 0;
		*var = value{}.set<value_type::string>(static_cast<const char*>(new_string), true);
		ret  = true;
	} else {
		ret = var->set(val);
	}

	for (auto& callback : _callbacks) {
		if (callback.name == name) {
			callback.operation->call(val, {old_val});
		}
	}

	return ret;
}

void globals_impl::internal_observe(hash_t name, callback_base* callback)
{
	_callbacks.push() = Callback{name, callback};
	if (_globals_initialized) {
		value* p_var = _variables.get(name);
		inkAssert(
		    p_var != nullptr,
		    "Global variable to observe does not exists after initiliazation. This variable will "
		    "therofe not get any value."
		);
		callback->call(p_var->to_interface_value(lists()), ink::nullopt);
	}
}

void globals_impl::initialize_globals(runner_impl* run)
{
	// If no way to move there, then there are no globals.
	if (! run->move_to(hash_string("global decl"))) {
		_globals_initialized = true;
		return;
	}

	// execute one line to startup the globals
	run->getline_silent();
	_globals_initialized = true;
}

void globals_impl::gc()
{
	// Mark all strings as unused
	_strings.clear_usage();
	_lists.clear_usage();

	// Iterate runners and mark their strings
	auto iter = _runners_start;
	while (iter != nullptr) {
		iter->object->mark_used(_strings, _lists);
		iter = iter->next;
	}

	// Mark our own strings
	_variables.mark_used(_strings, _lists);

	// run garbage collection
	_strings.gc();
	_lists.gc();
}

void globals_impl::save()
{
	_visit_counts.save();
	_variables.save();
}

void globals_impl::restore()
{
	_visit_counts.restore();
	_variables.restore();
}

void globals_impl::forget()
{
	_visit_counts.forget();
	_variables.forget();
}

snapshot* globals_impl::create_snapshot() const { return new snapshot_impl(*this); }

size_t globals_impl::compute_snapshot_size() const
{
	snapshot_interface::snapper snapper(strings(), _owner->string(0));
	bool                        migratable = can_be_migrated();
	size_t                      runner_cnt = 0;

	size_t length = snap(nullptr, snapper);
	for (auto node = _runners_start; node; node = node->next) {
		length += node->object->snap(nullptr, snapper);
		migratable = migratable && node->object->can_be_migrated();
		++runner_cnt;
	}
	if (migratable) {
		length += _owner->list_meta_size();
	}

	return snapshot_impl::file_size(length, runner_cnt, migratable);
}

size_t globals_impl::stream_snapshot_to(snapshot::writer& w) const
{
	snapshot_interface::snapper snapper(strings(), _owner->string(0));
	bool                        migratable = can_be_migrated();

	// Pass 1: dry run to calculate sizes
	size_t globals_size = snap(nullptr, snapper);

	// Collect runner sizes (stories in inkcpp generally have 1 runner, max a few)
	static constexpr size_t MAX_RUNNERS = 16;
	size_t runner_sizes[MAX_RUNNERS];
	size_t runner_cnt = 0;
	for (auto node = _runners_start; node; node = node->next) {
		if (runner_cnt >= MAX_RUNNERS) {
			return 0;
		}
		runner_sizes[runner_cnt] = node->object->snap(nullptr, snapper);
		migratable = migratable && node->object->can_be_migrated();
		++runner_cnt;
	}

	size_t list_meta_sz = migratable ? _owner->list_meta_size() : 0;

	// Total length calculation
	size_t payload_len = globals_size;
	for (size_t i = 0; i < runner_cnt; ++i) {
		payload_len += runner_sizes[i];
	}
	payload_len += list_meta_sz;

	size_t total_length = snapshot_impl::file_size(payload_len, runner_cnt, migratable);

	// Write Header
	snapshot_impl::header hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.num_runners = static_cast<uint32_t>(runner_cnt);
	hdr.length      = static_cast<uint32_t>(total_length);
	hdr.hash        = _owner->hash();
	hdr.migratable  = migratable;
	hdr.version     = snapshot_impl::SNAPSHOT_VERSION;

	if (!w.write(&hdr, sizeof(hdr))) {
		return 0;
	}

	// Write Lookup Table
	size_t num_offsets = runner_cnt + 1 + (migratable ? 1 : 0);
	uint32_t offsets[MAX_RUNNERS + 2];
	uint32_t off = static_cast<uint32_t>(sizeof(hdr) + num_offsets * sizeof(uint32_t));
	offsets[0] = off;
	off += static_cast<uint32_t>(globals_size);
	for (size_t i = 0; i < runner_cnt; ++i) {
		offsets[1 + i] = off;
		off += static_cast<uint32_t>(runner_sizes[i]);
	}
	if (migratable) {
		offsets[1 + runner_cnt] = off;
	}

	if (!w.write(offsets, num_offsets * sizeof(uint32_t))) {
		return 0;
	}

	// Stream Globals payload
	if (globals_size > 0) {
		unsigned char* buf = new (std::nothrow) unsigned char[globals_size];
		if (!buf) {
			printf("[inkcpp] stream_snapshot_to: FAILED to alloc globals buf (%u bytes)\n",
				(unsigned)globals_size);
			return 0;
		}
		snap(buf, snapper);
		bool ok = w.write(buf, globals_size);
		delete[] buf;
		if (!ok) {
			return 0;
		}
	}

	// Stream Runner payloads
	size_t ri = 0;
	for (auto node = _runners_start; node; node = node->next, ++ri) {
		size_t rsz = runner_sizes[ri];
		if (rsz > 0) {
			unsigned char* buf = new (std::nothrow) unsigned char[rsz];
			if (!buf) {
				printf("[inkcpp] stream_snapshot_to: FAILED to alloc runner buf (%u bytes)\n",
					(unsigned)rsz);
				return 0;
			}
			node->object->snap(buf, snapper);
			bool ok = w.write(buf, rsz);
			delete[] buf;
			if (!ok) {
				return 0;
			}
		}
	}

	// Stream List Metadata (already resident in memory)
	if (migratable && list_meta_sz > 0) {
		if (!w.write(_owner->list_meta(), list_meta_sz)) {
			return 0;
		}
	}

	return total_length;
}

bool globals_impl::can_be_migrated() const
{
	return _visit_counts.can_be_migrated() && _strings.can_be_migrated() && _lists.can_be_migrated()
	    && _variables.can_be_migrated();
}

size_t globals_impl::snap(unsigned char* data, const snapper& snapper) const
{
	unsigned char* ptr = data;
	inkAssert(_num_containers == _visit_counts.capacity(), "Should be equal!");
	inkAssert(
	    _globals_initialized,
	    "Only support snapshot of globals with runner! or you don't need a snapshot for this state"
	);
	ptr = snap_write(ptr, _turn_cnt, data != nullptr);
	ptr += _visit_counts.snap(data ? ptr : nullptr, snapper);
	for (unsigned i = 0; i < _visit_counts.capacity(); ++i) {
		ptr = snap_write(ptr, _owner->container_data(i)._hash, data != nullptr);
	}
	ptr += _strings.snap(data ? ptr : nullptr, snapper);
	ptr += _lists.snap(data ? ptr : nullptr, snapper);
	ptr += _variables.snap(data ? ptr : nullptr, snapper);
	return static_cast<size_t>(ptr - data);
}

const unsigned char* globals_impl::snap_load(const unsigned char* ptr, const loader& loader)
{
	_globals_initialized = true;
	ptr                  = snap_read(ptr, _turn_cnt);
	ptr                  = _visit_counts.snap_load(ptr, loader);
	size_t old_capacity  = _visit_counts.loaded_capacity();
	// shuffle values if needed
	if (loader.migratable) {
		// extend array if needed
		if (_visit_counts.capacity() < _owner->num_containers()) {
			_visit_counts.resize(_owner->num_containers());
		}
		_visit_counts.save();
		for (size_t i = 0; i < old_capacity; ++i) {
			_visit_counts.set(i, visit_count());
		}
	}

	inkAssert(
	    _visit_counts.capacity() >= _owner->num_containers(),
	    "Missmatching number of tracked containers."
	);
	for (size_t i = 0; i < old_capacity; ++i) {
		hash_t path;
		ptr = snap_read(ptr, path);
		if (! loader.migratable) {
			inkAssert(i < _owner->num_containers(), "Container index exceeds story containers");
			inkAssert(path == _owner->container_data(static_cast<container_t>(i))._hash, "Container hash mismatch in snapshot");
		} else {
			container_t c_id         = ~0U;
			ip_t        container_ip = path != 0 ? _owner->find_offset_for(path) : nullptr;
			bool        found        = container_ip != nullptr
			          && _owner->find_container_id(
			              static_cast<uint32_t>(container_ip - _owner->instructions()), c_id
			          );
			if (found) {
				_visit_counts.set(c_id, _visit_counts.get_old(i));
			}
		}
	}
	if (loader.migratable) {
		_visit_counts.forget();
		_visit_counts.resize(_num_containers);
	}
	inkAssert(
	    _num_containers == _visit_counts.capacity(),
	    "errer when loading visit counts, story file dont match snapshot!"
	);
	ptr = _strings.snap_load(ptr, loader);
	ptr = _lists.snap_load(ptr, loader);
	ptr = _variables.snap_load(ptr, loader);
	return ptr;
}

bool globals_impl::migrate_new_globals(
    const loader& loader, globals_impl& new_globals, const char* list_metadata
)
{
	if (! _variables.migrate(new_globals._variables)) {
		return false;
	}
	if (_lists && ! loader.old_ref_table) {
		if (! _lists.create_match_lut(
		        list_metadata, loader.list_list_matches, loader.list_value_matches, loader.old_ref_table
		    )) {
			return false;
		}
	}
	if (_lists) {
		_lists.init_static_list_flags(_owner->lists(), _variables);
		if (! _lists.migrate_variables(
		        loader.list_old_new_map, loader.list_list_matches, loader.list_value_matches,
		        *loader.old_ref_table, _variables
		    )) {
			return false;
		}
	}
	return true;
}

config::statistics::global globals_impl::statistics() const
{
	return {
	    _variables.statistics(), _callbacks.statistics(), _lists.statistics(), _strings.statistics()
	};
}

} // namespace ink::runtime::internal
