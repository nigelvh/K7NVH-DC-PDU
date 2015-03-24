board_x = 115;
board_y = 66.1;
board_z = 14;
board_thickness = 1.8;

board_spacing = 2;

hole_diameter = 3.5;
screw_hole_diameter = 3;
screw_head_diameter = 8;
post_diameter = 6;
post_base_height = 3;
pin_height = 1.7;
pin_diameter = 2;

wall_thickness = 2;

tab_width = 15;
tab_length = 15;
tab_hole_diameter = 4.5;

peg1_x = -54.85;
peg1_y = 30.48;
peg2_x = -54.85;
peg2_y = -30.48;

peg3_x = 54.85;
peg3_y = 30.48;
peg4_x = 54.85;
peg4_y = -30.48;

peg5_x = -54.85;
peg5_y = 15.3;
peg6_x = 54.85;
peg6_y = 15.3;
peg7_x = -54.85;
peg7_y = -15.3;
peg8_x = 54.85;
peg8_y = -15.3;
peg9_x = -54.85;
peg9_y = 0;
peg10_x = 54.85;
peg10_y = 0;

air_hole_width = 10;
air_hole_height = 10;
air_hole_gap = 10;

powerpole_hole_width = 17;
powerpole_hole_height = 9;
powerpole_input_x = 0;
powerpole_input_y = 25.25;
powerpole_p1_x = 48.5;
powerpole_p1_y = 22.9;
powerpole_p2_x = 48.5;
powerpole_p2_y = 7.6;
powerpole_p3_x = 48.5;
powerpole_p3_y = -7.6;
powerpole_p4_x = 48.5;
powerpole_p4_y = -22.9;
powerpole_p5_x = -48.5;
powerpole_p5_y = 22.9;
powerpole_p6_x = -48.5;
powerpole_p6_y = 7.6;
powerpole_p7_x = -48.5;
powerpole_p7_y = -7.6;
powerpole_p8_x = -48.5;
powerpole_p8_y = -22.9;

// Bottom Plate
difference(){
	cube([board_x + board_spacing + wall_thickness*2, board_y + board_spacing + wall_thickness*2, wall_thickness], center=true);

	// Screw holes and countersinks
	translate([peg1_x, peg1_y, -(wall_thickness/2)]){
		cylinder(h = post_base_height+wall_thickness, r = hole_diameter/2, $fn=20);
		cylinder(h = wall_thickness, r1 = screw_head_diameter/2, r2 = hole_diameter/2, $fn=20);
	}
	translate([peg2_x, peg2_y, -(wall_thickness/2)]){
		cylinder(h = post_base_height+wall_thickness, r = hole_diameter/2, $fn=20);
		cylinder(h = wall_thickness, r1 = screw_head_diameter/2, r2 = hole_diameter/2, $fn=20);
	}
	translate([peg3_x, peg3_y, -(wall_thickness/2)]){
		cylinder(h = post_base_height+wall_thickness, r = hole_diameter/2, $fn=20);
		cylinder(h = wall_thickness, r1 = screw_head_diameter/2, r2 = hole_diameter/2, $fn=20);
	}
	translate([peg4_x, peg4_y, -(wall_thickness/2)]){
		cylinder(h = post_base_height+wall_thickness, r = hole_diameter/2, $fn=20);
		cylinder(h = wall_thickness, r1 = screw_head_diameter/2, r2 = hole_diameter/2, $fn=20);
	}
}

// Mounting Tabs
translate([(board_x + board_spacing + wall_thickness*2)/2 - 0.1 + tab_length/2,(board_y + board_spacing + wall_thickness*2)/2 - tab_width/2,0]){
	difference(){
		#cube([tab_length,tab_width,wall_thickness], center=true);
		cylinder(h = wall_thickness, r = tab_hole_diameter/2, center=true);
	}
}
translate([(board_x + board_spacing + wall_thickness*2)/2 - 0.1 + tab_length/2,-((board_y + board_spacing + wall_thickness*2)/2 - tab_width/2),0]){
	difference(){
		#cube([tab_length,tab_width,wall_thickness], center=true);
		cylinder(h = wall_thickness, r = tab_hole_diameter/2, center=true);
	}
}
translate([-((board_x + board_spacing + wall_thickness*2)/2 - 0.1 + tab_length/2),(board_y + board_spacing + wall_thickness*2)/2 - tab_width/2,0]){
	difference(){
		#cube([tab_length,tab_width,wall_thickness], center=true);
		cylinder(h = wall_thickness, r = tab_hole_diameter/2, center=true);
	}
}
translate([-((board_x + board_spacing + wall_thickness*2)/2 - 0.1 + tab_length/2),-((board_y + board_spacing + wall_thickness*2)/2 - tab_width/2),0]){
	difference(){
		#cube([tab_length,tab_width,wall_thickness], center=true);
		cylinder(h = wall_thickness, r = tab_hole_diameter/2, center=true);
	}
}

// Peg 1
translate([peg1_x, peg1_y, wall_thickness/2]){
	difference(){
		cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
		cylinder(h = post_base_height+wall_thickness, r = hole_diameter/2, $fn=20);
	}
}
// Peg 2
translate([peg2_x, peg2_y, wall_thickness/2]){
	difference(){
		cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
		cylinder(h = post_base_height+wall_thickness, r = hole_diameter/2, $fn=20);
	}
}
// Peg 3
translate([peg3_x, peg3_y, wall_thickness/2]){
	difference(){
		cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
		cylinder(h = post_base_height+wall_thickness, r = hole_diameter/2, $fn=20);
	}
}
// Peg 4
translate([peg4_x, peg4_y, wall_thickness/2]){
	difference(){
		cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
		cylinder(h = post_base_height+wall_thickness, r = hole_diameter/2, $fn=20);
	}
}

// Peg 5
translate([peg5_x, peg5_y, wall_thickness/2]){
	cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
	translate([0,0,post_base_height]){
		cylinder(h = pin_height, r = pin_diameter/2, $fn=20);
	}
}
// Peg 6
translate([peg6_x, peg6_y, wall_thickness/2]){
	cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
	translate([0,0,post_base_height]){
		cylinder(h = pin_height, r = pin_diameter/2, $fn=20);
	}
}
// Peg 7
translate([peg7_x, peg7_y, wall_thickness/2]){
	cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
	translate([0,0,post_base_height]){
		cylinder(h = pin_height, r = pin_diameter/2, $fn=20);
	}
}
// Peg 8
translate([peg8_x, peg8_y, wall_thickness/2]){
	cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
	translate([0,0,post_base_height]){
		cylinder(h = pin_height, r = pin_diameter/2, $fn=20);
	}
}
// Peg 9
translate([peg9_x, peg9_y, wall_thickness/2]){
	cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
	translate([0,0,post_base_height]){
		cylinder(h = pin_height, r = pin_diameter/2, $fn=20);
	}
}
// Peg 10
translate([peg10_x, peg10_y, wall_thickness/2]){
	cylinder(h = post_base_height, r = post_diameter/2, $fn=20);
	translate([0,0,post_base_height]){
		cylinder(h = pin_height, r = pin_diameter/2, $fn=20);
	}
}

// -X Wall
translate([-(board_x + board_spacing + wall_thickness)/2,0,(board_z + wall_thickness)/2 + wall_thickness/2]){
	cube([wall_thickness, board_y + board_spacing + wall_thickness*2, board_z + wall_thickness], center=true);
}
// +X Wall
translate([(board_x + board_spacing + wall_thickness)/2,0,(board_z + wall_thickness)/2 + wall_thickness/2]){
	cube([wall_thickness, board_y + board_spacing + wall_thickness*2, board_z + wall_thickness], center=true);
}
// -Y Wall
difference(){
	translate([0,-(board_y + board_spacing + wall_thickness)/2,(board_z + wall_thickness)/2 + wall_thickness/2]){
		cube([(board_x + board_spacing + wall_thickness*2), wall_thickness, board_z + wall_thickness], center=true);
	}
	translate([0,0,wall_thickness/2 + air_hole_height*2/2]){
		#cube([air_hole_width*5, (board_y + board_spacing + wall_thickness*2), air_hole_height*2], center=true);
	}
	translate([(air_hole_width+air_hole_gap)*2,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([(air_hole_width+air_hole_gap)*3,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([-(air_hole_width+air_hole_gap)*2,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([-(air_hole_width+air_hole_gap)*3,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
}
// +Y Wall
difference(){
	translate([0,(board_y + board_spacing + wall_thickness)/2,(board_z + wall_thickness)/2 + wall_thickness/2]){
		cube([(board_x + board_spacing + wall_thickness*2), wall_thickness, board_z + wall_thickness], center=true);
	}
	translate([0,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([(air_hole_width+air_hole_gap)*1,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([(air_hole_width+air_hole_gap)*2,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([(air_hole_width+air_hole_gap)*3,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([-(air_hole_width+air_hole_gap)*1,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([-(air_hole_width+air_hole_gap)*2,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
	translate([-(air_hole_width+air_hole_gap)*3,0,wall_thickness/2 + air_hole_height/2]){
		#cube([air_hole_width, (board_y + board_spacing + wall_thickness*2), air_hole_height], center=true);
	}
}

// Top Plate
// Lid on top
//translate([0,0,wall_thickness*2 + board_z +1]){
//rotate([180,0,180]){
// Lid flat for printing
translate([0,100,0]){
rotate([0,0,180]){
difference(){
	cube([board_x + board_spacing + wall_thickness*2, board_y + board_spacing + wall_thickness*2, wall_thickness], center=true);

	// Powerpole holes
	translate([powerpole_input_x,powerpole_input_y,0]){
		rotate([0,0,90]){
			cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
		}
	}
	translate([powerpole_p1_x,powerpole_p1_y,0]){
		cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
	}
	translate([powerpole_p2_x,powerpole_p2_y,0]){
		cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
	}
	translate([powerpole_p3_x,powerpole_p3_y,0]){
		cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
	}
	translate([powerpole_p4_x,powerpole_p4_y,0]){
		cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
	}
	translate([powerpole_p5_x,powerpole_p5_y,0]){
		cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
	}
	translate([powerpole_p6_x,powerpole_p6_y,0]){
		cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
	}
	translate([powerpole_p7_x,powerpole_p7_y,0]){
		cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
	}
	translate([powerpole_p8_x,powerpole_p8_y,0]){
		cube([powerpole_hole_width, powerpole_hole_height, wall_thickness], center=true);
	}
}

// Peg 1
translate([peg1_x, peg1_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = screw_hole_diameter/2, $fn=20);
	}
}
// Peg 2
translate([peg2_x, peg2_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = screw_hole_diameter/2, $fn=20);
	}
}
// Peg 3
translate([peg3_x, peg3_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = screw_hole_diameter/2, $fn=20);
	}
}
// Peg 4
translate([peg4_x, peg4_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = screw_hole_diameter/2, $fn=20);
	}
}

// Peg 5
translate([peg5_x, peg5_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = hole_diameter/2, $fn=20);
	}
}
// Peg 6
translate([peg6_x, peg6_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = hole_diameter/2, $fn=20);
	}
}
// Peg 7
translate([peg7_x, peg7_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = hole_diameter/2, $fn=20);
	}
}
// Peg 8
translate([peg8_x, peg8_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = hole_diameter/2, $fn=20);
	}
}
// Peg 9
translate([peg9_x, peg9_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = hole_diameter/2, $fn=20);
	}
}
// Peg 10
translate([peg10_x, peg10_y, wall_thickness/2]){
	difference(){
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = post_diameter/2, $fn=20);
		cylinder(h = wall_thickness + board_z - post_base_height - board_thickness, r = hole_diameter/2, $fn=20);
	}
}
}
}