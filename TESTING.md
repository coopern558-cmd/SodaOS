# SodaOS Testing Checklist

## Environment
- [ ] WSL2 Ubuntu installed
- [ ] Required packages installed
- [ ] QEMU available

## Build
- [ ] `make clean` succeeds
- [ ] `make build` succeeds
- [ ] `make iso` succeeds

## Boot
- [ ] `make run-sdl` opens QEMU
- [ ] SodaOS desktop appears
- [ ] No immediate crash/hang

## Navigation
- [ ] Up/Down selection works
- [ ] Enter opens selected app
- [ ] `1..4` quick-open works
- [ ] Esc returns to desktop

## Terminal
- [ ] `help` works
- [ ] `ls` works
- [ ] `cat /home/readme.txt` works
- [ ] `cat /home/notes.txt` works
- [ ] `mem` works
- [ ] `clear` works

## Notes + RAMFS
- [ ] Typing in Notes works
- [ ] Backspace works
- [ ] Notes visible via terminal `cat /home/notes.txt`

## Stability
- [ ] 3 consecutive boots succeed
- [ ] 2+ minutes of app switching without crash
