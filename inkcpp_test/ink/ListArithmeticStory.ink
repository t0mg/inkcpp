LIST first_list = (first_item), second_item
LIST locations = (at_shore), in_shallows, in_open_sea, in_deep_sea
LIST items = on_floor, (in_boat), dropped_in_sea

VAR current_loc = in_shallows
VAR oar_state = in_boat

-> start

=== start ===
~ current_loc++
{current_loc:
- in_open_sea: far from shore, open sea
- else: not open sea
}
~ oar_state = dropped_in_sea
+ {oar_state ? (in_boat)} [Row further] -> start
+ {oar_state ? dropped_in_sea} [Wait] -> done

=== done ===
Done.
-> END
