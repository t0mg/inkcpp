LIST Inventory = (Key), (Torch), (Amulet)

-> start

=== start
~ temp items = Inventory
-> offerItem(items)

=== offerItem(items)
~ temp item = pop(items)
Item: {item}
{ items: -> offerItem(items) }
-> DONE

=== function pop(ref _list)
~ temp el = LIST_MIN(_list)
~ _list -= el
~ return el
