# joymon
a keybind daemon for joysticks

## idea
inspired by [vim-clutch](https://github.com/alevchuk/vim-clutch) i wanted to control my computer with my flight sticks. so i made a daemon to do so.

## usage

this is the config that i wrote this for:

```
#main stick
joystick /dev/input/js0;
axis 2 x -1 exec i3-msg -t command 'focus left';
axis 2 x 1 exec i3-msg -t command 'focus right';
axis 2 y -1 exec i3-msg -t command 'focus up';
axis 2 y 1 exec i3-msg -t command 'focus down';

#throttle
joystick /dev/input/js1;
button 1 down exec i3-msg -t command 'workspace prev';
button 2 down exec i3-msg -t command 'workspace next';

```

to find out what mappings you need run joymon in raw mode with

```
./joymon -r
```

this will print out the current keys and events so you can move your hardware around and find what events you want to bind to.

the joystick directive specifies what all following directives will be bound to and it is specified before buttons and axis.

```
joystick {path};
```

for binding buttons the format is as simple as:

```
button {number to use} {event type} exec {command to run};
```

like wise for axis

```
axis {number to use} {x,y} {minimum activation tolerance} exec {command to run};
```

if you feel the need to comment you can

```
# yay comments
```

## usage info

```
joymon - a simple daemon to respond to joystick events
-[-d]aemonize - run in the background
-[-r]aw - dump all event information to the console. useful for finding keycodes
-[-t]est - run as normal but dont daemonize. useful for finding command errors
-[-c]onfig - defaults to XDG_CONFIG_HOME/joymon/config
-[-h]elp - this message
```
