#ifndef UG2_DEFAULT_CFG_H_
#define UG2_DEFAULT_CFG_H_

// Text of the settings file written on the first start; every line is a comment, so nothing changes until edited.

namespace ug2 {

const char DefaultSettingsFile[] =
    "# NFS Underground 2 for the PS Vita: settings.\n"
    "# Each setting below is shown at its default. To change one, remove the # at the start of its line and\n"
    "# edit the value, then start the game again. Delete this file to get a fresh copy. Pressing START on the\n"
    "# start-up screen opens a menu that edits this file for you.\n"
    "\n"
    "# The start-up screen with its 3 second countdown (START configures, any other button skips): on or off.\n"
    "#show_configurator_on_boot=on\n"
    "\n"
    "# Frame rate: 30 locks to 30 frames a second for steady pacing, 0 unlocks it.\n"
    "#frame_rate=30\n"
    "\n"
    "# Rear touch pad as L2, R2, L3 and R3 (unbound until set in the Controls screen): on or off.\n"
    "#rear_touch=on\n"
    "\n"
    "# Right stick turns the car in the garage and the customisation menus: on or off.\n"
    "#garage_stick=on\n"
    "\n"
    "# Dead zone of the right stick in the garage and the customisation menus, in percent (0 to 70);\n"
    "# raise it if the car spins by itself.\n"
    "#garage_stick_deadzone=30\n"
    "\n"
    "# Dead zone of the right stick for free look, in percent (0 to 70); raise it if the camera\n"
    "# moves by itself.\n"
    "#free_look_deadzone=30\n"
    "\n"
    "# Steering and throttle stick: pushes within this many degrees of the stick's up/down (throttle)\n"
    "# or left/right (steering) axis count by how far the stick is pushed, so a full push slightly\n"
    "# off-axis still gives full throttle or steering. 0 is the PC game's behaviour: only the axis counts.\n"
    "#stick_cone_angle=25\n"    "\n"
    "# Scenery narrower than this many pixels on screen uses its simpler model. 0 is off; 64 by default.\n"
    "# Higher values give more speed and more pop-in.\n"
    "#dev_world_lod_pixels=64\n"
    "\n"
    "# Scenery smaller than this many pixels on screen is not drawn. 0 is off, 64 the most; 8 by default. More speed,\n"
    "# more pop-in.\n"
    "#dev_world_min_pixels=8\n"
    "\n"
    "# on skips the menus and starts a test race.\n"
    "#boot_race=off\n"
    "\n"
    "# Cheaper tyre smoke and other particles, close to the original picture: on or off (off is as the original).\n"
    "#optimised_particles=on\n"
    "\n"
    "# Music decoded by the Vita's hardware MP3 decoder instead of the game's own decoder, which frees the CPU:\n"
    "# on or off. It sounds the same; if the hardware decoder cannot start, the game's decoder plays it.\n"
    "#music_hw_decode=on\n"
    "\n"
    "# The sound mixer holds the game's sound lock for a shorter time, so the game waits on it less: on or off.\n"
    "#audio_mixer_split=on\n"
    "\n"
    "# Sounds quieter than this many dB below full scale are not mixed, which saves processor time; off mixes\n"
    "# every sound. -60 is inaudible; louder values such as -40 save more.\n"
    "#audio_silence_db=-60\n"
    "\n"
    "# on writes a detailed log to NeedForSpeedUnderground2.log, useful when reporting a problem. Errors\n"
    "# are logged either way.\n"
    "#log_info=off\n";

}

#endif /* !UG2_DEFAULT_CFG_H_ */
