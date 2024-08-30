In the late 90's / early 2000's, I bought a Forte VFX1 headgear VR headset second hand from Cash Converters. This came with a hand device they called a Cyberpuck. Both the VFX1 
and the Cyberpuck used the Earths magnetic field to determine movement. In the VFX1 this determined where the wearer was looking and in the 
case of the Cyberpuck, what hand movements the user was making.

![Cyberpuck](https://github.com/user-attachments/assets/cb6fa4a1-0dd3-47e8-b7d8-5924772b9e01)
<i>Original Cyberpuck from 1995. (Wikipedia free use image)</i>

![cropped image Cyberpuck in hand](https://github.com/user-attachments/assets/b0caff30-a8c0-4885-aa3f-7ad38ffe92b6)
<i>My version of the cyberpuck 2024</i>

This is my version of the Cyberpuck, upgraded to be wireless. I was inspired to develop this based on an Element14 video I watched where Clem
tries to make a mouse using an IMU sensor but ends up with something you have to pick up and twist and tilt to get working.  I though, that is 
like the old Cyberpuck I used to have.

The Cyberpuck is a 2DOF mouse that uses pitch to move the mouse pointer up and down the screen and roll to move it left and right. I can't 
remember how I used the original version but for this one, the mouse only moves if the lower button (GPIO2) is pressed. This is so you can set
it down with the mouse pointer moving unintentionally. The top button (GPIO4) provides the left mouse button functionality while the middle
button (GPIO3) is the right mouse button.

<b>Parts List:</B>
<ul>
<li>XIAO ESPC3 micro controller</li>
<li>GY-271 - Compass sensor</li>
<li>3 6x6x5mm tactile switches</li>
<li>connecting wire</li>
<li>lipo battery with builtin BMS protection circuit of an appropriate size to fit in the case. I used a 501220.  
As you can see from the photos, there is space for a larger battery.</li>
<li>You will also need the 3D printed parts, 1 each of the side pieces and 3 of the buttons</li>
</ul>

<b>Wiring Diagram:</b>

![wiring diagram](https://github.com/user-attachments/assets/72778985-5421-4704-95ee-939d0b3efd30)


I bought all the parts except the battery from Amazon. The battery I bought from eBay.

I chose the XIAO ESP32C3 microcontroller because it was being advertised in my Google news feed at the time and I liked the small size. It has a
hardware UART (serial interface) which means that it can't simulate a mouse over the USB cable so Bluetooth has to be used. If you don't want to 
use the battery, it can be powered over USB but you still need to use Bluetooth to use it as a mouse. Other XIAO size micro controllers can be 
used but you'll need to modify the code if the pinouts are different.

![photo of completed wiring](https://github.com/user-attachments/assets/b53cd8d0-f0eb-489c-82bf-9ea4ea08af85)


Keep the wires short when wiring things up so that everything fits inside the case.

![photo of circuit in case](https://github.com/user-attachments/assets/14deca58-d31c-4e4c-ab7b-afaa710c524d)

There is a lip to the right hand side of the bottom case where the battery is. The top lid has a matching part which slips below this lip and
using the USB access hole as a guide to ensure the two halves aren't twisted fold the two halves together. There are protrusions on the top
half of the case that fit down the side of the switches and hold it closed.

Once built download the sketch from this repositary page. The code uses two libraries. The QMC5883LCompass library so that the microcontroller can 
use the GT-271 sensor and a BLEMouse library so it can act as a Bluetooth mouse. Download the one at 
https://github.com/sirfragles/ESP32-BLE-Mouse/tree/dev The official BLEMouse library fails to compile but this version has a fix installed.

To minimise battery drain when the mouse isn't being used, the ESP32C3 will go into a deep sleep state after 2 minutes of non use. Pressing 
any of the buttons will wake it up again. The battery is recharged by plugging the microcontrollers USB port into any 5v USB power source such as a computer or power bank .

With the Cyberpuck mouse awake, Open the Bluetooth settings on whatever device you want to use it with and scan for new devices. “Cyberpuck mouse”
should appear in the list as a mouse device. Connect to it, and holding the device in your hand with the middle buttons pointing forward and the
USB connector below, press the button connected to GPIO2 (the bottom button in my case and the bottom button in the wiring diagram), and move 
your arm up, tilting from the elbow and the mouse pointer should go up, tilting the device to the side should move the pointer to the left or 
right depending on the direction of the tilt.

Here is a short demonstration followed by the device being assembled.

https://youtu.be/AyjnlBCEP1o

I don't see this replacing the traditional mouse for word processing or spreadsheet work but more as a gaming or presentation control device. I
think it would be good in a simple flight simulator or as a weapon in a simple first person shooter. I have had fun playing online mouse games,
but I keep losing to people using a traditional mouse. Maybe it is because I need more practice (or maybe I'm crap at the games).

<b>Additional Information</b>

The GY-271 compass magnetometer uses a QMC5883L digital compass chip. A lot of the listings for this sensor board list it as having a HMC5883L
chip. Some versions of this board also have HMC5883L written on them.
This caused me to lose a lot of time as both chips offer a similar function but have different I2C address. This resulted in me getting zeros back from the I2C interface. The chip on the pcb just had 5883 written on it.
I downloaded a sketch that allowed me to see which addresses were active on the I2C bus and from that I found out that the chip
must have been a QMC5883L.

The QMCX5883Lcompass library has an example file for calibrating the chip. This tries to get the maximum and minimum values while you wave the sensor
around 360 degrees in all directions. I tried this and also captured large volumes of X, Y and Z values into a spreadsheet to analyse it. The
values given from the three directional sensors on the chip have different maximum values and different maximum values in the negative and
positive directions. This is why calibration is necessary if you need accurate direction values.

I stopped this project for several months as I had to think how to convert a 3D vector to the mouse movement I wanted. It turned out I didn't need
to as all we need is a relative change. We only need 2 DOF for this. So provided the mouse is rotated around one of its axis, the relative change in
value will give you the direction and amount.

Given the orientation of the board in the case, a roll (tilt left or right) cause a change in the Y and Z axis readings . 
A pitch change causes a change in the X and Y axis readings. For the porpose of using this as a mouse we only need to look at the change in Z readings to determine how much to move the mouse pointer in the horizontal and in what direction. The X reading is used in the same way for 
vertical movements. We can ignore the Y readings.

I intended this to be a wired mouse so I was surprised to find that I couldn't do that because the microcontroller I chose had a hardware UART to
communicate over USB. This means that it couldn't send mouse emulation signals over that port. I'd have to change the microcontroller or use
bluetooth. So bluetooth it was. That also meant that there was no point using USB just for power. So I'd need to add a battery and battery saving
features. Such as detecting when the device has been idle for a period and putting it in a power saving mode and be able to wake up from that.

Mapping the values from the compass sensor to mouse movements was also an interesting challenge. We want the mouse pointer to move fast across the screen
but still be able to do delicate movements. For this reason there is an exponential type mapping between the compass sensor and the steps the mouse will take.
Small tilt movements take small steps but large tilt movements cause large steps to be taken. There is a wider range of movement allowed for
small steps than large steps. This is achieved by having a mapping table that contains more small steps and as the movement gets larger and
the steps get bigger, the number of entries in the mapping table get smaller. 

The mapping table is sized based on the sum of integers up to a certain number. For ease of giving an example, i'll use the number 4. In this
case the mapping table would have 10 entries (4+3+2+1 = 10), The first 4 entries in the mapping table are filled with 1, the second 3 are filled with 2 the third 2 are filled with 3 and the last one is filled with 4. These numbers represent the number of steps the mouse will take
if that mapping entry is selected. We then need to take a reading from the compass, multiple that by 10 and divide the result by what we think is the largest number the compass can return (- the deadzone). Convert that into an integer and you should get a number between zero and ten. that we can then use to look up a mouse movement amount in the mapping table.

I might add something to the code to emulate even smaller steps. Such as emulating half steps by making the mouse only take a step every second time through the main loop for very small mouse movements.
