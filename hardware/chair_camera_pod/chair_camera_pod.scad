// Concealed DFRobot DFR1154 V1.1 chair camera pod, prototype v1.
// Strap beneath the walkway-side wicker armrest; +Z faces the walkway.
// Board and optic positions MUST be verified on the physical V1.1 unit.
part = "tray"; // "tray", "cover", or "assembly"
$fn = 64;

module rounded_slot_x(x0, x1, y, diameter, z0, height) {
    hull() {
        translate([x0,y,z0]) cylinder(d=diameter,h=height);
        translate([x1,y,z0]) cylinder(d=diameter,h=height);
    }
}

module rounded_slot_y(x, y0, y1, diameter, z0, height) {
    hull() {
        translate([x,y0,z0]) cylinder(d=diameter,h=height);
        translate([x,y1,z0]) cylinder(d=diameter,h=height);
    }
}

module tray() {
    difference() {
        union() {
            // Square rear plate and short side/top shroud. Bottom stays open
            // for the board's USB-C plug and a downward cable drip loop.
            translate([-27,-27,0]) cube([54,54,3]);
            translate([-27,-27,3]) cube([3,54,10]);
            translate([24,-27,3]) cube([3,54,10]);
            translate([-27,24,3]) cube([54,3,10]);
            // Four board spacers (3 mm high).
            for (x=[-17,17], y=[-17,17])
                translate([x,y,3]) cylinder(d=7,h=3);
            // Two rear ears accept soft hook-and-loop straps threaded through
            // the wicker frame. They stay behind the camera's front plane.
            translate([-39,-18,0]) cube([13,36,4]);
            translate([26,-18,0]) cube([13,36,4]);
        }
        // Adjustable PCB slots accommodate 30-38 mm horizontal hole pitch.
        for (y=[-17,17], s=[-1,1])
            rounded_slot_x(s*15,s*19,y,3.0,-1,9);
        // Strap slots in the ears.
        for (x=[-33,33])
            rounded_slot_y(x,-11,11,5,-1,6);
        // M2 cover screws: pilot bores into the side-wall corner lands.
        for (x=[-25.5,25.5], y=[-23,23])
            translate([x,y,5]) cylinder(d=1.6,h=9);
    }
}

module cover() {
    difference() {
        translate([-27,-27,0]) cube([54,54,2.4]);
        // Approximate source-board layout: lens ~6 mm above board center;
        // two IR emitters on each side. Kept as separate unglazed openings
        // so the IR cannot reflect from a shared close window into the lens.
        translate([0,6,-1]) cylinder(d=18,h=5);
        for (x=[-15.5,15.5])
            rounded_slot_y(x,0,11,7,-1,5);
        // The four side-wall screws secure a removable faceplate.
        for (x=[-25.5,25.5], y=[-23,23])
            translate([x,y,-1]) cylinder(d=2.4,h=5);
    }
}

if (part == "tray") tray();
else if (part == "cover") cover();
else { tray(); translate([0,0,13]) cover(); }
