# Arduino-Stopwatch
Stopwatch on Display. Arduino + Funshield.

It is a homework for Computer Systems course.

# Description 
Design and implement a Stopwatch based on Arduino UNO with attached Funshield. 
The stopwatch must measure the time internally in milliseconds and display the measured values on the 7-segment LED display with 0.1s precision (including the decimal dot, rounding down).
The number should be displayed in regular decadic format used in Czech Republic (no leading zeros except one right before the decimal dot).
The stopwatch is zeroed at the beginning (i.e., displaying only 0.0 aligned right).

Stopwatch Controls
The stopwatch is always in one of three logical states (stopped, running, or lapped). In the stopped state, internal counter is not advanced and the display is showing the last measured value (this state is also the initial state after boot).
In the running state, the internal clock is measuring passed time and the value on the display is updated continuously. In the lapped state, the stopwatch is still running (collecting time), but the display is frozen -- showing the value which was actual when the lapped state was initiated.
The transition diagram looks as follows: 
[Image]

- Button 1 performs the start/stop function. It has no bearing in the lapped state.
- Button 2 freezes/unfreezes (laps/un-laps) the time. It has no bearing in the stopped state.
- Button 3 works only in the stopped state and it resets the stopwatch (sets internal time counter to 0).

See this video, which visualize the reference solution, to get a better idea: https://www.youtube.com/watch?v=wT15zxqQthM&feature=youtu.be

Important: The recodex is testing the clock to a greater precision than the stopwatch is actually showing on its 7-segment display. It does things like stopping the clock at 0.12s from the beginning, expecting to see 0.1 on the display,then starting them again for 50 ms. On the second stop it expects to see 0.1 (the internal time is 0.17 now), then it runs it again for, say, 0.131s and expects 0.3 on the display.

Warning: If you decide to implement button debouncing as well (not explicitly required here), please bear in mind that this assignment is tested beyond human capabilities in ReCodEx. Namely, we are using rather extreme timing for button actions. However, you can rely on the fact that the shortest button downtime or uptime (i.e., time between two subsequent button events) would be at least 50ms.

We would encourage you to divert your effort and attention to the correct code decomposition so you separate the display management and the stopwatch application (and the buttons handling) as much as possible. That way you could re-use some parts of your code in the future assignments.
