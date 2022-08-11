<center>

# **Noah Corona**

## **ECE 153A Lab:** *Chromatic Tuner*

</center>

![gif-fpga-chromatic-tuner](https://user-images.githubusercontent.com/25698069/184216645-29b99c44-e32e-4934-b9d1-98a35f3fae36.gif)


## **Program Features**
The chromatic tuner uses the onboard microphone of the Artix-A7 to identify any note played from the 3rd to the 9th octave. When a note is played into the microphone, the closest matching note is displayed on the LCD. To the right of the note, the error from this note is displayed in cents with the appropriate sign value. Below this, the error bar displays the error from this note and the direction of the error in a more visually friendly way. If the note played closely matches the nearest note, the error bar turns green, making it easy to determine when tuning is complete. It can measure notes in octaves 3-8 to an accuracy of +/- 10 cents. By default, it uses the frequency detected by the microphone to automatically select the best effective sampling frequency. There is also a menu to manually select a desired octave, which can be used to achieve the same effect. Additionally, there is a menu allowing the user to adjust the frequency of the note A4. By default, A4 is tuned to 440 Hz, but may be set anywhere between 420 and 460 Hz.

## **Menus and UI**
The background and window colors utilize gray and white, while all text is printed in a bold red color. The red text easily contrasts with the window background colors, and the windows contrast well with the screen’s background color. Overall, this made the program results easier to interpret given the limited space on the LCD.

![image](https://user-images.githubusercontent.com/25698069/184216055-b0073d91-d9cf-4b1a-8aae-016aa635131b.png)


The ‘note’ window is always visible while the program runs, and will update while in either of the two menus. In the ‘note’ window the user sees the closest note to the detected frequency, as well as the calculated error from this note. Below this window, the error bar gives a graphical representation of the error, turning green when a note is closely matched.

![image](https://user-images.githubusercontent.com/25698069/184216426-557fe21a-df66-4837-8184-7cbbf71b67af.png)


The first of the menus allows for tuning the frequency value of the note A4. In the ‘Tune A4’ menu, the current value and new value of the A4 frequency are displayed in Hz. This menu can be accessed by pressing the up push button, or with a counterclockwise twist of the rotary encoder. The new value may be increased or decreased using the left and right push buttons or by twisting the rotary encoder counterclockwise or clockwise. To confirm the new value, either press the center push button or press on the rotary encoder. To close the menu without applying the new selection, press the up push button again. This frequency value for A4 may not be set lower than 420 Hz or higher than 460 Hz.

![image](https://user-images.githubusercontent.com/25698069/184216368-04f8d73e-8b88-4794-be0a-66b9996ce741.png)


The second menu is used to select an octave to be used for tuning. In the ‘Select Octave’ menu, the current and new value for the desired octave are displayed in the window. The menu can be accessed by pressing the down push button or with a clockwise twist of the rotary encoder. The selection can be changed using the left and right push buttons, as well as clockwise/counterclockwise twists of the rotary encoder. The new selection is applied with a press of the center push button or press of the rotary encoder. To close the menu without applying the new selection, press the down push button. A selection of ‘A’ causes the program to determine the correct octave automatically.

There is also a variable ‘showDetails’ in the main program file that, when set to 1, will cause the frequency computed by the FFT as well as the program loop time to be printed on the bottom of the LCD.

## **Extra Features**
I first chose to add the automatic ranging feature. This works by detecting the octave last played into the microphone, and adjusting the effective sample rate to achieve the most accurate result from the FFT in this octave. To implement auto-ranging, many configurations of effective sample rates were tested while supplying a variety of notes from different octaves as inputs to the mic. The accuracy and program time of each configuration was recorded and the results compared. Using this process, I identified which configurations best identified notes for each octave in a reasonable amount of program time. Because the code for identifying the note/octave was included in the project files, all that needed to be done was the implementation of the effective sample rate selection.

Because it was useful for visually testing the program, I chose to also implement the error bar. The note error is computed regardless whether this feature is implemented, so adding this feature was simply a matter of adding the appropriate graphical functions and function calls. Because the feature adds value to the program and incurs relatively little overhead, I felt that not implementing it would have been a waste. I considered adding the red/green LED indicators to add further visual functionality to the program. Instead, I chose to change the error bar color from red to green when a note is closely matched, allowing all of the program information to be clearly visible in one place and achieving the same effect.


## **Testing Process**
The tone generator used to test the program was made by Tomasz Szynalski and is available [here](https://www.szynalski.com/tone-generator). For each octave (3 to 9), the appropriate frequency for each note was played. The tone was played through ear buds, and both ear buds placed near the microphone pin hole of the Artix-A7 board. With perfect program
 
accuracy, the error for a perfectly matched note would be zero. However, because there are multiple sources of noise in our measurements, we see non-zero error for many notes that are perfectly matched. The program is accurate to +/- 10 cents, meaning the error for each note will not exceed 10 cents in either direction when the input frequency falls between the 4th and 8th octaves. In the 3rd octave, notes are spaced very closely together, differing by just a few Hz in frequency. Because low-frequency noise is present, the performance in the 3rd octave is rather poor. Even a 3 Hz measurement error in the 3rd octave translates to double-digit error in cents, making it difficult to correctly tune in this octave. However, the program is capable of correctly identifying notes in the third octave. Thankfully, the accuracy of the program increases greatly as the frequency of the input increases, with many octaves averaging less than 5 cents of error when perfectly matched notes are played. Similarly, the program’s performance in the 9th octave is also suboptimal. Beyond the note F in the 9th octave, whether due to the chosen sampling rate or because of limitations of the microphone, the program cannot consistently identify a note with a reasonable amount of error.


## **Program Structure**
The program uses a combination of a grand loop and interrupt-driven events. This design approach allows the display to update with new information while in any stage of any menu, without ever preventing the user from providing input. The grand loop accounts for most of the program’s functionality. First, the loop selects the appropriate number of samples to collect, as well as the effective sample frequency, based on either the last detected frequency or on the manually-selected octave. Then, it collects the appropriate number of samples from the microphone stream, and uses these samples to compute the FFT. The resulting frequency is then used to determine the closest note and octave. Then, the difference between the note played and the closest note is calculated in cents. Finally, the new note, octave, and error text values are reprinted, and the error bar redrawn.
The event-driven portions of the code are the menu controls. The on-board Artix-A7 push buttons and rotary encoder are the two forms of user input, both generating interrupt signals that update the relevant input values or menu statuses, and redrawing the LCD as necessary. The interrupt handler functions for the push buttons and encoder largely matched those of previous labs. Both the on-board push buttons and the rotary encoder push button utilize a half-second debounce timer, while the rotary encoder twists are debounced using a finite state machine. Because of this, the rotary encoder is capable of accepting successive twist inputs very quickly.


## **Optimizations**
To quickly recognize a frequency, optimizations had to be made to the code provided. From the last lab, we knew that most of these optimizations center around the longest portion of the code: the FFT computation. To speed this up, the most notable optimization was the use of lookup tables. Two lookup tables were implemented for the repeating sin and cosine factors that appear in our complex multiplications. This eliminated the need to recompute recurring sin and cosine factors, trading this time instead for the access of an array value. For large FFT computations, where many complex multiplications will reoccur, this saves a drastic amount of execution time. Next, portions of the code which were redundant were removed. For instance, the complex multiplication process was initially performed independently of the final reordering process. This meant having two separate loops that traverse two arrays in the same order. By performing the reordering immediately after the complex
multiplication process, we eliminate the need for the second loop entirely. Some portions of code were moved closer to their references with the intention of reducing cache misses. While further optimizations certainly exist in the code, the program execution time was short enough to provide a fluid user experience for the desired functionality.

