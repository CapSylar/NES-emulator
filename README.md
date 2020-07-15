# NES-emulator
a simple NES emulator written in C, has a debugging interface that displays the pattern tables

### Building
- to build the project run ```cmake CMakeLists.txt``` and then ```make <makefileName>```

### working on 
- apu
- refactoring the project 
- perfecting the ppu
- debugging interface

### implemented mappers
- mapper 0
- mapper 2
- mapper 4
### Screenshots
<img src="/screenshots/mario.png" /> | <img src="/screenshots/balloon_fight.png" /> 

### Bugs
- ppu has timing issues, some games lock up for some reason.
- Games run too fast, but this is intentional.