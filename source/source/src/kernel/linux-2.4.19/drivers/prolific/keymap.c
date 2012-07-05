
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
// #include <linux/irq.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/kbd_ll.h>
#include <linux/kbd_kern.h>
#include <linux/timer.h>
#include <linux/unistd.h>

/*
keycode  30 = a
keycode  48 = b
keycode  46 = c
keycode  32 = d
keycode  18 = e
keycode  33 = f
keycode  34 = g               
keycode  35 = h               
keycode  23 = i               
keycode  36 = j               
keycode  37 = k               
keycode  38 = l               
keycode  50 = m               
keycode  49 = n               
keycode  24 = o               
keycode  25 = p               
keycode  16 = q               
keycode  19 = r               
keycode  31 = s               
keycode  20 = t               
keycode  22 = u               
keycode  47 = v               
keycode  17 = w               
keycode  45 = x               
keycode  21 = y               
keycode  44 = z               
*/
int a_z_to_scancode[]={
  30,
  48,
  46,
  32,
  18,
  33,
  34,           
  35,         
  23,         
  36,         
  37,              
  38,            
  50,            
  49,         
  24,              
  25,          
  16 ,           
  19,               
  31 ,           
  20 ,          
  22,          
  47 ,          
  17,             
  45,           
  21,           
  44 ,
};

static int num_to_scancode[]={
  11,
  2,
  3,
  4,
  5,
  6,
  7,           
  8, 
  9,
  10

};

int ascii_to_scancode(int key){
	if (key>='a' && key <='z'){
		return a_z_to_scancode[key-'a'];
	}else if (key>='0' && key <='9'){
		return num_to_scancode[key-'0'];
	}else if (key=='-'){
		return 12;
	}else if (key=='='){
		return 13;

	}
	return 0;

}

