set pri pre on
target extended-remote :3333
p $pc += 4
b vchiq_handmade

source gdb.helpers
