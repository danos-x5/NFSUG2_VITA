#ifndef UG2_DEFAULT_CFG_H_
#define UG2_DEFAULT_CFG_H_

// Text of the settings file written on the first start; every line is a comment, so nothing changes until edited.

namespace ug2 {

const char DefaultSettingsFile[] =
    "# NFS Underground 2 for the PS Vita: settings.\n"
    "# Each setting below is shown at its default. To change one, remove the # at the start of its line and\n"
    "# edit the value, then start the game again. Delete this file to get a fresh copy.\n"
    "\n"
    "# Frame rate: 30 locks to 30 frames a second for steady pacing, 0 unlocks it.\n"
    "#frame_rate=30\n"
    "\n"
    "# Picture: on fills the 16:9 screen with the HUD at the edges, off is the original 4:3 with black bars.\n"
    "#widescreen=on\n"
    "\n"
    "# Graphics preset: vita is what the port is tuned for, max turns everything on and is much slower.\n"
    "# The game's own Display menu changes the individual options; above Medium detail the Vita struggles.\n"
    "#gfx_preset=vita\n"
    "\n"
    "# Rear touch pad for shifting and the camera: on or off.\n"
    "#rear_touch=on\n"
    "\n"
    "# Front touch screen as a mouse click in the menus (no cursor is drawn): on or off.\n"
    "#front_touch=on\n"
    "\n"
    "# Right stick turns the car in the garage and the customisation menus: on or off. The dead zone is a\n"
    "# percentage of the stick's range; the speed is the camera movement at full deflection (mouse counts per frame).\n"
    "#garage_stick=on\n"
    "#garage_stick_deadzone=15\n"
    "#garage_stick_speed=4\n"
    "\n"
    "# Scenery narrower than this many pixels on screen uses its simpler model. 0 is off; 128 gives more\n"
    "# speed, and higher values show more pop-in.\n"
    "#world_lod_pixels=0\n"
    "\n"
    "# Scenery smaller than this many pixels on screen is not drawn. 0 is off, 64 the most. More speed,\n"
    "# more pop-in.\n"
    "#world_min_pixels=0\n"
    "\n"
    "# on skips the menus and starts a test race.\n"
    "#boot_race=off\n"
    "\n"
    "# Cheaper tyre smoke and other particles, close to the original picture: on or off (off is as the original).\n"
    "#optimised_particles=on\n"
    "\n"
    "# on writes a detailed log to NeedForSpeedUnderground2.log, useful when reporting a problem. Errors\n"
    "# are logged either way.\n"
    "#log_info=off\n";

}

#endif /* !UG2_DEFAULT_CFG_H_ */
