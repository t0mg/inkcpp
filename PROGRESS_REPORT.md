# Inkcpp Pass-By-Reference and List Operation Progress Report

## Summary
The goal was to investigate and fix a crash occurring in `inkcpp` when passing an Ink list by reference to a function (e.g. `pop(ref _list)`). The codebase was throwing an `Unhandled ink runtime exception: Could not find variable!` due to a failure to resolve call-index pointers properly.

## Bugs Investigated and Fixed

### 1. The Variable Reference Crash (`Unhandled ink runtime exception: Could not find variable!`)
**Root Cause:**
Variables passed by reference inside functions and knots are wrapped in a `value_pointer` holding a call index (`ci`) and the variable `name`. The logic inside `runner_impl.cpp` (`get_var` and `set_var`) to resolve these pointers was superficial. It only handled `ci == 0` (global scope), and if passed recursively to another function or re-referenced, it failed to trace the pointer chain down to the original frame in `_stack`, leading to a missing variable error.

**Fixes Applied:**
*   Patched `inkcpp/runner_impl.cpp` inside `get_var<Scope::LOCAL>` to repeatedly follow pointer references inside `_stack.get_from_frame(ci, name)` until the concrete list value is found.
*   Patched `set_var<Scope::LOCAL>` to similarly resolve the pointer chain recursively and mutate the exact origin variable via `origin->redefine(val, _globals->lists())` instead of overwriting the `value_pointer` in the stack.

### 2. List Item `LIST_MIN` Return Value Defect
**Root Cause:**
While testing the fix for the infinite loop `Item: Torch`, we found that `LIST_MIN(items)` for `(Key, Torch, Amulet)` incorrectly returned `Torch` instead of `Key`. In `list_table::min` and `list_table::max` inside `inkcpp/list_table.cpp`, the resulting `list_flag` was wrongly using the *numeric value* of the flag (e.g. `_flag_values[j]`) instead of the required *flag index offset* (`j - listBegin(i)`). This broke all internal conversions through `toFid(e)`.

**Fixes Applied:**
*   Modified `list_table::min` and `list_table::max` to assign `res.flag = static_cast<int16_t>(j - listBegin(i));` ensuring it conforms to the proper struct format mapping correctly back to string labels and flag indexes.

### 3. List Element Subtraction Mutating List Unsafely
**Root Cause:**
When a list element was removed via `_list -= el` (e.g. `Command::SUBTRACT`), the operation hit `list_table::sub`. Inside `sub`, instead of mutating the newly generated output list container, the method wrongly called `setList(l, rh.list_id, false);` where `l` was a pointer to the original input list definitions container. This corrupted the lists.

**Fixes Applied:**
*   Patched `list_table::sub` in `inkcpp/list_table.cpp` to correctly alter the output list memory (`o`) rather than the original memory reference (`l`), preserving immutability constraints.

## Testing Setup
Added a formal test in `inkcpp_test/Lists.cpp` that loads `ListPopRef.ink`. This explicitly confirms `LIST_MIN` correctly pops sequential elements while evaluating truthful pointer recursion behavior correctly without infinite looping. All 50 tests currently compile and succeed (with threading issues properly cleared).
