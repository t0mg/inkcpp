#include "catch.hpp"
#include "system.h"

#include <../runner_impl.h>
#include <choice.h>
#include <compiler.h>
#include <globals.h>
#include <runner.h>
#include <story.h>

using namespace ink::runtime;

SCENARIO("TagsAndBranching", "[tags][branching]")
{
	GIVEN("A story with tags and branching")
	{
		story* _ink    = story::from_file(INK_TEST_RESOURCE_DIR "TagsAndBranching.bin");
		runner _thread = _ink->new_runner();

		WHEN("Starting the thread")
		{
			CHECK_FALSE(_thread->has_tags());
			CHECK_FALSE(_thread->has_knot_tags());
			CHECK(_thread->get_current_knot() == 0);
		}

		WHEN("On the plain text line")
		{
			CHECK(_thread->getline() == "Plain text\n");
			THEN("It has tags")
			{
				CHECK(! _thread->has_knot_tags());
				CHECK(_thread->has_tags());
				REQUIRE(_thread->num_tags() == 1);
				REQUIRE(std::string(_thread->get_tag(0)) == "plain_text_tag");
			}
		}

		WHEN("In the knot")
		{
			// Skip previous test
			_thread->getline();
			CHECK(_thread->getline() == "Knot text\n");
			THEN("It has tags")
			{
				CHECK(_thread->get_current_knot() == ink::hash_string("Knot"));
				CHECK(_thread->has_knot_tags());
				REQUIRE(_thread->num_knot_tags() == 1);
				REQUIRE(std::string(_thread->get_knot_tag(0)) == "knot_tag");
				CHECK(_thread->has_tags());
				REQUIRE(_thread->num_tags() == 2);
				REQUIRE(std::string(_thread->get_tag(0)) == "knot_tag");
				REQUIRE(std::string(_thread->get_tag(1)) == "knot_text_tag");
			}
		}

		WHEN("In the tunnel")
		{
			// Skip previous tests
			_thread->getline();
			_thread->getline();
			CHECK(_thread->getline() == "Tunnel text\n");
			THEN("It has tags")
			{
				CHECK(_thread->has_knot_tags());
				REQUIRE(_thread->num_knot_tags() == 1);
				REQUIRE(std::string(_thread->get_knot_tag(0)) == "tunnel_tag");
				CHECK(_thread->has_tags());
				REQUIRE(_thread->num_tags() == 2);
				REQUIRE(std::string(_thread->get_tag(0)) == "tunnel_tag");
				REQUIRE(std::string(_thread->get_tag(1)) == "tunnel_text_tag");
			}
		}

		WHEN("In the thread")
		{
			// Skip previous tests
			_thread->getline();
			_thread->getline();
			_thread->getline();
			CHECK(_thread->getline() == "Thread text\n");
			THEN("It has tags")
			{
				CHECK(_thread->has_knot_tags());
				REQUIRE(_thread->num_knot_tags() == 1);
				REQUIRE(std::string(_thread->get_knot_tag(0)) == "thread_tag");
				CHECK(_thread->has_tags());
				REQUIRE(_thread->num_tags() == 2);
				REQUIRE(std::string(_thread->get_tag(0)) == "thread_tag");
				REQUIRE(std::string(_thread->get_tag(1)) == "thread_text_tag");
			}
		}
	}
}
