---
subject: embedded-systems
type: concept
topic: Project briefing transcript
---

# Embedded Systems project briefing — formatted transcript

- **Speaker:** Muhamed Fauzi Bin Abbas
- **Recording date shown in export:** 31 August 2026, 2:48 PM
- **Recording duration shown in export:** 35 minutes
- **Original export reference:** `project-briefing-zoom-transcript.txt` (not included in this repository).

> Formatting edition: Zoom interface text and the duplicate speaker label are omitted. Spoken text is retained in order, including repetitions, recognition errors and corrections. Section headings and paragraph breaks are editorial. Only one speech timestamp (00:05) was supplied; no later timestamps have been invented. This edition was checked against the supplied text, not against the recording.

## Reading notes

The transcript repeatedly renders “hump” as “hum”, “harm”, “hub” or “arm”, and appears to misrecognise micro-T-Kernel, FreeRTOS, PID, “buddy” and “peer evaluation”. These are contextual interpretations, not audio-verified corrections. The speaker corrects the GY511 description from gyroscope to magnetometer. An obstacle passage says both “straight line” and “curved line”; earlier passages explicitly say the obstacle is on a straight section, not a curve. Confirm the written brief if placement matters.

## Transcript

**Muhamed Fauzi Bin Abbas · 00:05**

### Welcome and autonomous mission

Okay, guys. So welcome to the Embedded Systems Project Briefing. So the challenge… is to design, develop, integrate, and test an automatic… oh, sorry, an autonomous robotic car that can complete a predefined course. Without a remote control. So it's supposed to be autonomous. So the, the… in this course, people… Combine… or in this project, we'll be combining the line following, the directional barcodes. The speed harms of different heights, and the static obstacle.

So, the important word is autonomous. Again, the robot must collect sensory information, decide what to do, and act. Using the onboard. Embedded software that you need to write. The, the project is therefore not just about, what do I call Sorry. It's not just about making a car move, but it's to bring together, you know, how do you design this embedded software? How do you integrate the sensor, then how do you make a decision in real time?

and put it all together. So, at the end… The aim is to have one complete system that is demonstrable Of this whole entire project. So, in this slide, we are summarizing, the complete mission, which is, first, is the robot must move continuously to follow the line. Second, it must detect a barcode, which should be either on the left or right. It'll be fixed, when you… by the time you get the kit. The… and then decode the navigation command, meaning that the barcode will tell you whether you want… you need to turn right at the cross junction, turn left.

to go forward, or to do a U-turn. So then it must also detect a hum. The hum will be only for one wheel, not the double hump, because else there's too many equipment sensors at the front that it will block. So it will be only for one wheel. They will go up the hump. And you need to be able to detect calculate the height of the hum, based using the… accelerometer… Spencer… Okay?

Then, a fourth is that you need to, detect the obstacle. Which is, will be blocking the line itself, so when it detects there's an obstacle on the line, the ultrasonic sensor, which is mounted on the servo, will scan it, will scan left and right to find out You know, what is the width of the… the obstacle, then make a decision whether you want to go to the left or go to the right. So then it will take the route out of the line, meaning that you won't have any line to guide you, you have to go around the obstacle, then join back into the line, okay? So some of the questions that some of your previous seniors have asked is that,

Will it happen in a curve? No, it will happen in a straight line. So the obstacle will happen in a straight line, so that at least you know Approximately when to… to join back. And finally, the robot, must report useful telemetry data over Wi-Fi, to be more specific, MQTT, and the assessment will be considered navigation accuracy, reliability, quad quality, and efficiency and robust of the system, okay?

So a successful run will therefore depend on, you know, the behavior and not just the completing one functional call itself. Okay.

### Required real-time operating system

So, in addition to that, I forgot to mention that one of the requirements, which is different from your seniors, is that I'm enforcing that you must use a micro-T kernel on the Raspberry Pi Pico. So there's a requirement, actually. If you want to use free outdoors, you can. But then you'll be kept at the… You'll be penalized for that, a little bit, okay? So you need to use micro-T kernel. I will not teach you micro-cheek enough. Okay? But I will have a lab on free RTOS. So the idea is that learning a new RTOS is part and parcel of this whole, entire thing. So you need to figure out

No, you know how FreeROS works? you try to map the kernel itself on the Raspberry Bi Pico. Okay? So just keep that in mind. Of course, the project, the repository will be shared, so that you'll be able to download a basic Raspberry, sorry, the basic Micro T kernel, real-time operating system, or a TOS, running on the Pico. Then you can start writing your own application. Of course, if you have any issues, by all means, ask. Yours truly, if you need any help.

Be it in a discussion group or face-to-face, okay? So, you might want to try it as soon as possible, once you have the PCOR up and running, okay? Hmm…

### Team size and subsystem ownership

What else? So, each buddy will… so there's 5 people in a group. Again, this project briefing is specifically for the default project. Okay?

I will come to the non-default project after this. So, for the default project, there'll be always 5 members, and each member needs to take hold of one part of the subsystems, okay? Take ownership of the subsystem. So, body number 1 will handle Wi-Fi and telemetry, body number 2 will handle motion control. Buddy number three is the line following and the barcode. Line number four is IMU and the terrain, and line number 5 is the obstacle. So, if some of you have been talking to your seniors, there's a slight change compared to last year. Number one, there's no remote control. Number two,

we'll be using, the terrain. There'll be the hump, which is not there last year, but it was there about… 3 or 4 years ago, so I'm bringing back the terrain into this, and the other thing which is different is the enforcement of a real-time operating system. So prior to this, your seniors doesn't I was not required to use a real-time operating system, but I realized that Many of you who don't struggle with… with the testing and debugging. So by enforcing it on a micro-T kernel, real-time operating system, it helps with, you know.

With your testing, helps you with the isolation of problems, and helps you with the coordination between tasks itself. Okay? And for the fact that PICO has Wi-Fi, makes the easiest way to put it all together holistically, okay?

### Subsystem 1: Wi-Fi, MQTT and telemetry

So, budget number one, Wi-Fi communication command and telemetry. So, the idea that you must use MQTT, And the idea is that the client must be able to collect all the information pertaining the car, you know, like the movement of the car, how many harms have you detected, the height of the harm. whatever barcode have you scanned through, what was the decoder messages, whether the car is moving straight, left, or right, all these things… Sign needs to be put together and send it out through MQTT to the broker, and then, the broker, I will have a document on how you set up a broker on your laptop, but on the day itself of the demo, we'll have our own broker, so you have to connect to our broker itself.

Then you can, then you are able to see the telemetry data. The output of the telemetry data doesn't need to be fanciful. Actually, to be able… it's just a matter of sending out the messages. Just make sure that the messages that is being sent out, that is seen by the publisher… the subscriber on the PC itself. It's clear, so that, you know, it's… when you read it, you understand what's happening, actually. Okay?

what do you call it? What else? Telemetry data, communication, yeah. So, there should also be a way for you to send certain command to the car. So, for example, like, suddenly I see, like, because the telemetry data can come out as and when. But suddenly, if I want to say, okay, give me a summary of all the telemetry data, you should be able to send a request, and the car should be able to send back, you know, a snapshot of what's happening at that point of time.

upon request. So there should… there must be a two-way communication, meaning that the… the client itself must be able to handle, To receive messages, but also being able to send messages, too, at the same time. Okay.

### Subsystem 2: motion control and PID

The next thing is the motion control, so how the car is… So, you need to be able to ride the drivers for the motors, you need to be able to do a PID, which is to… to regulate the speed control, because no two motors are the same. Which means that, no matter how expensive of a motor you buy, no two motors are exactly the same. So you need to be able to write an algorithm that will try to make sure that both motors are moving at the same speed. So you may be thinking, sir, why don't I just put the same data on both wheels? Unfortunately, like I said.

I can put one… PWM speed on one motor, and the same one on the second motor, but the second motor might move faster. Well, the first motor might move slower, it depends, and it's not the same. So you need to create an algorithm that will try to find the error between the two, and try to eliminate them. And one thing you notice, once… and the PID will… you need to identify and tune the PIND. These three numbers, needs to be trained on… needs to be identified based on your

Kind, so… how you want the motors to move. You want to be aggressive, or you want to be not aggressive, it depends on how you want to… the behavior of your motors. These numbers will not be transferable to the next group. So once you tune your PID for your car, you'll be forever fixed to your car. Meaning that my advice, when you want to do tuning, make sure The wheels are all fixed. no more changing, then you tune the PID. Okay?

So, in addition to that, must also be able to move the car accurately, meaning we can move the car in a straight line, you can turn it to the left at any precise angle, 90 degrees, 45 degrees, and whatnot. So on and so forth, okay?

Also, when you're going up the hump, one will be higher than the other, so you need to… yet you are still following the… the line, how do you handle that? How do you manage that? So those are the things that the motor controller guys do handle.

### Subsystem 3: line following and barcode decoding

Next is the barcode decoding and IR line following. Last year, I only allowed two IR sensors, one for the line following, one for the barcode. This year, I… because a lot of students have requested, that one line was too difficult. I decided to allow two IR sensors for the line following. And the third one for the barcode. So, this is just an example of how it might look like. So, how you place the sensor, I leave it up to you. The idea is that you have two IR sensors to detect the line, YR sensor to detect the barcode along the way.

Okay? So the barcode needs to be read in two directions. The barcode is directional, meaning that if you scan one way and scan another way, you have different data. But your algorithm must be able to Decode the barcode regardless of which direction it's scanning. Okay? It has been done before, so it's not impossible. And plus the fact that now you have two IR sensors to detect the line, so you can balance the two very well, as compared to your seniors. Oh, by the way, the seniors managed to solve the problem, so they were able to do line following with one IR sensor, but again,

I felt that, I kind of felt guilty for you guys, so I'm allowing two IR sensor in this case.

### Subsystem 4: IMU and hump measurement

So, the next thing is body number 4 is the accelerometer, the IMU-based motion sensing. So, you need to be able to write the driver for it. Technically, the GY511 have two sensors inside it. One is the accelerometer, one is a gyroscope. So, one will tell you, the acceleration, it can tell you, I don't know, whether it's going up the slope, going down the slope, whether it's saying it, things like that. Plus, it's able to also tell you the direction you're looking at, the directional. That's where the magnetometer comes in. Sorry, it's not a gyroscope, it's a magnetometer. So it has an accelerometer and a magnetometer, so it has a…

Digital Compass. And, the ability to have acceleration. So, with those two. sensors inside the GY511, you are able to tell whether what is the orientation of the car, what is the direction the car is moving, so it will… this is the part where you need to work with the car and the motor guys to make sure when you turn, you can, you know… say, okay, you know, we are moving, you know, it's only 45 degrees, move somewhere, move somehow, no, until you get the 90 degrees. So you have two sets of data, one from the wheel encoder, one from the… this IMU sensor, to get you the direction and bearing, also.

And this applies to many other parts. So, you can work on each part individually, but later on, you might need to merge the sensor data to get better accuracy, and that's what I'll be looking out for. So, if we just use the gyroscope purely for, you know, for, Measuring the harm, and that's it, then you won't… you wouldn't get better marks as compared to those who actually do a sensor fusion, where they combine different sensors together to… to get better accuracy and robustness

Okay? So, even by the tilt also, you can tell you what is the height of the harm. Then you say, sir, how do we measure the height of the harm? By the tilt. is by practical, things. So you, you tilt the car, you, you measure the height, the maximum height, you say, okay, when you tilt it, you measure what is the, all the different senses, the reading, you know? You tilt it higher, means higher harm, what is the measurement? So then you map The sensor data to the height of the…

To the height of the hub. So that's how you… then you can do simple arithmetic to be able to calculate it. So when… so once you get enough height readings. you can actually come up with a simple algorithm that will just give you the sensor data, the IMU sensor data, and you can estimate what is the possible height of the hub itself. Then, of course, you need to tune it after a while. So, it is possible, again, this was done. 5 years ago, I think, and you were able to get outstanding accuracy based on this. So it can be done. Of course, you cannot do it at very high speed, so probably you need to work with your motor guys, say, hey, if you detect a harm, I will tell you to slow down. You slow down, then we got time to measure properly, you know, to measure the height of the harm.

whether you want to stop and measure, or you want to, you know, move and measure, I leave it up to you. Of course, those who move and measure will get much better marks, as compared to those who just, you know, stop and measure, you know, stop and measure, stop and measure. Okay?

Yeah.

### Subsystem 5: ultrasonic scanning and obstacle avoidance

The next and the final one will be actually the adaptive ultrasonic scanning and obstacle profiling. So, which means that you move on the line itself, then suddenly there'll be one big monolith blocking your way. Then you say, oh… Tom, what do I do? Okay, then you take the ultrasonic sensor. And the servo is scanned to the left. Okay, to the right, on the left-hand side, we've got a bigger gap. Okay, let me take the left-hand side. Okay, we'll go to the left-hand side.

Then it will continue scanning a little bit, you know, to find out what is the depth of the obstacle. Then once it realized that you passed the obstacle, it will turn back in and follow the line. Again, like I mentioned earlier, the obstacle will be on a straight line, it will be on a curved line, so it makes the algorithm slightly easier. Okay?

Oh, what else? Yeah, so you need to measure the width of the obstacle itself, and send the data to the Wi-Fi. So, once you measure the width, approximate… and remember, when you measure the width, doesn't mean you move… tune-tune the angle, you have to give it some space. Remember, you're a car, you have a big width, so you need to give enough Space for you to move. And this is the part where the PID is very important, because if the motor control cannot go straight, then God bless you, because then you won't be able to make that curve very nicely. I mean, not saying it's impossible, but it's going to be a lot more challenging, okay? Again, the map for the final day

is undetermined yet, so it'll be a surprise for you. But there is a line for doing this, for sure, and the hum will be on one side, while the barcode will be on the other side. It is fixed. I will confirm it to you, by the time you get the equipment, so you don't have to worry about the harm and the barcode being on the same side. It's on the opposite side, and it'll be fixed, okay?

### Mission assessment and configurable barcode commands

Okay, so what do the team be assessed on, on the overall mission itself? Is it able to complete the whole entire course, measure the height of the harm, you know, follow the instructions on the barcode by decoding it properly, and move it, and move around the course, you know. Correctly, as required. The next thing is the navigation itself, whether it is accurate or not, you know? Does it follow when you scan the barcode and it asks you to U-turn, does it actually U-turn?

immediately or at the… at the junction. By right, it's supposed to be at the junction itself, you know? And the junction will always have… the barcode will always happen before the junction. So, if I scan, scan, scan, scan, if I move along, then I scan the barcode. Only when I meet the junction do I take action on whatever the barcode command is, actually, lah. And again, the barcode command will change on the day itself, meaning that you need to be able to decode for A to Z, but on the day itself, I'll tell you, okay, A is actually go straight, Z is actually go behind, and now W is actually go left, F is equal to turn right.

You know, something like that lah. So you need to be able to send a command through the Wi-Fi to reprogram this thing on the fly itself, and not recompile the whole code. So these are something that you need to be mindful of for. So that's another thing that you need to consider for the Wi-Fi. You need to be able to Change the barcode no, the command that you need to detect to take an action now. So… default ABCD, A is forward, B is left, C is right, and D is U-turn. But on the D itself, we will ask you for different, the alphabets will be different.

Okay? But it will still be forward, left, right, or U-turn. At the cross junction itself. Okay?

Yeah, so another thing is how robust the whole entire thing… you shouldn't be… Oh, it works now, then… then the next run, it doesn't work anymore, then it just means that it wasn't well tested. Unless, like, bearing major circumstances where somebody kicked the robot or whatever, lah, that is a different case altogether. But if the car is moving now, it should always be moving and always be working all the time, okay? So that's what you need to make sure that it works perpetually.

So testing, testing, testing is very important, okay? Hmm.

### Requirements changes and tribunals

Another thing would be the requirements change will be under the tribunal. Tribun 1, like I mentioned, I will give you a requirements change. And you need to incorporate it into the project in week 8. On week 13, of course, there'll be another requirement change. You do not have to incorporate it, but you need to still discuss it. So you see that, oh no, how do we implement this, you know, what needs to be changed? Put it in writing, in theory. So, the main purpose is to train you guys on how to be able to use AI to quickly pivot your project and identify the challenges, the problems, and how the new design needs to be

Given the requirements change. Then you'll be thinking, does this happen often? Trust me, it happens very often in the industry, a lot more than you would like to believe, okay? So, requirements change happens all the time. very rarely, you wouldn't have a requirements change. Only in the academic area, you don't have a requirements change, because we are quite… Prim and proper, but when you go to the industry, things change all the time, unfortunately. So keep that in mind. So this is a good way to, to kind of brush up how do you handle requirements change and change your design based on that, okay? So, with that.

### AI-generated code and individual understanding

again, few things when you want to develop. Again, most of the coding will be done by AI, I'm quite sure of it. Very rarely, I don't expect many of you to be writing code by hand anymore. I won't stop you, and I… I mean, I still recommend it to ride by hand if you can, but I don't expect most of you will do that. But… Make sure every code that you submit, you understand. So if I'm doing the Wi-Fi part, every single line I know what it does exactly. If randomly we ask you, what does this line do? You cannot answer.

I'll treat it, and you get penalty points for that, okay? So, you need to be able to own every single line of code, even though those are not written by you. I mean, if it's part of your subsystem, you should be able to answer them. Of course, if it's another subsystem, then the body who's in charge of that subsystem should be able to answer them, okay? Keep that in mind.

### Group grades, peer evaluation and contribution evidence

pertaining the grades, I think I forgot to mention. So, I will grade the project Holistically first. You will get a grade. After that, what was gonna happen is that I would look at a PI evaluation and say, oh. Body number 8, Deep. more work, and he helped buddy number two. So, contribution for buddy number one. will go high, body number 2 will go low, which means that, just say the project was initially a B-minus project, for example, overall, because buddy number 2 kind of messed up the whole thing.

It's a B- project. But because buddy number one helped buddy number two, the B-, he might get B, maybe even B+, depending on how terrible buddy number 2 is. Embody number 2. get a B minor, he might get even lower. C, C- maybe even, it depends. But this, moving of the grids will only affect the group work part, the whole entire group work part, okay? So please, do put in your effort, put in your time. If any of the members are slacking off, please,

Okay, in this case, you can email me, okay? If any of your members are really slacking off, email me, I will talk to the… then it can be anonymous if you need to. Let's see what we can do, but please email me early. Don't tell me at the last one or two weeks that, oh, this person slacked off, didn't do anything, this person's slack off, didn't do anything. I would prefer if you tell me early, okay? And again, during the peer evaluation, give… evidence that this person did something, this person didn't do something. Again, the whole project will be in the GitHub report, so I will track your commits, I will track your mergers,

as and when. So, please do a proper commit and… And push SNM into your repository. I will check who uploaded, who did not upload, and things like that. Okay, I won't track in real time. But… At the end of the day. Yeah, I will track it. So. Don't wait until the end, then push everything once a week, or once a day, depending, or things like that. And make sure you comment it accordingly. Okay?

### Opening the project write-up

Okay, so that is for this. So, let me show you an example, okay? Let me… The show… Let me open up the… Where is the documents?

Okay. Sorry. Hear me. Okay, so… No, wait, this is the wrong one. Give me a moment, my apologies. May Easter. I apologize, I'm still learning how to use this MacBook. First time I'm using it. Okay, there we go. So… I hope this is the correct one. So this is the write-up which I've uploaded about the autonomous car. There's one for…

### Completion time and demo attempts

So the one for the default project has it in great detail. So, you know, what do you need to do? Like, you need to follow the lines, you need to have a barcode that can be scanned in either direction, you need to be able to measure the hum, various heights, you know, you need to be able to measure the width of the obstacle that's blocking your way. So they're all there. then you need to, you know, be able to navigate accurately, you know. When you say completion time, not that the fastest time will work, but it shouldn't take too long. So if I… if it takes, like, what, 10 minutes to finish the whole entire course, then something is wrong. The whole demo should be able to…

In theory, you should be able to finish within less than 5 minutes, actually, if it does… if it's done well. So if it sticks… too long, we will take note of it, and there might be some penalty to it, actually. Because if you go very slow, of course, we can scan and sum everything, correct? So you should move in a reasonable speed. I will not mandate what is a reasonable speed, but, I mean, the whole… Entire demo should be, com- can be complete within, 5 minutes. I think that's a reasonable time.

During the final demo, we… We will allow 3 times, for you to try it out. But we will only take the most recent marks only. Meaning that first time you try, you get, let's say. And you get… All, you, you get most of the… the features, working. I say, okay, never mind. I get… and then you see, approximately, you get, no, 3 out of 5 of the thing working. I'll try again. You try again, you get, 2 out of 5 only, this time. One of them suddenly failed to work.

So, which means that your marks will be 2 out of 5, for example. But then you say, okay, I got my third chance. I'll throw in the dice, I'll try the third time. You try it the third time, if you get 1 out of 5, then 1 out of 5 is your final mark. But if you are lucky, if it works, we get 4 out of 5, then 4 out of 5 is your final mark. So, before you decide whether you want to go for the next try. think about it. So some groups in the… previously, once they got 3 out of 5, or something like that, they will just say, I'm happy, sir, I'm stopping here.

Okay? So, please, test. you can have confidence if you have tested it properly. You will test, test, test, test, then you will know whether your system is able to do that correctly or not. Sometimes, unfortunately. Things happen, but that's why we have 3 rounds, 3 trials. So if you test it properly and it works all the time, first time might be a fluke, but second and third time, it will always work. So make sure testing is done rigorously, okay?

### Navigation telemetry and changing barcode mappings

Okay, what else? Yeah, so then, when you decode the barcode, it can go… forward, left, right, or turn during a cross junction. So when you hit a cross junction, you need to make a decision to go straight, left, right, or U-turn from there, okay? You should also be able to detect the height of the arm, you know, and measure the height, you'll be able to detect the obstacle, and also you need to be able to or, like, I won't say map the whole map, but, you know, you need to be able to kind of

build it up, meaning, like, okay, I'm moving forward, and I move forward for, what, for 20 CM. Then I turn, right? So you can actually literally kind of guess how the whole map will look like. So you need to be able to kind of come up with the telemetry of how the whole thing will be like, and at what point of time, where are the obstacles, where are the barcodes, all that needs to be captured in the telemetry data. Oops, it easy. Okay, where do you put it?

what else? Then you have the… oh, yeah, so the barcode, when you… okay, this is ABCD, it's just for testing purposes, but during the final demo itself, we will tell you what is the correct term for… to turn left, right, straight, or U-turn. What does it mean? It means that you need… from A to Z, all the barcodes, patterns need to be stored in your… Yeah, PICO itself. So that on the day itself, you just need to send a command, say, oh, maybe on the day itself, DCBA…

D will be the turn left, you know, C, I can flip it around, and then the whole code must work immediately, okay? So you don't have time to recompile. We will not allow the code to be recompiled once you know what is the… the barcosa. Okay, what else? Anything else I need to exercise?

### Development and integration advice

Yeah, again, my suggestion is to right drivers for each one of your devices, then I would then start looking at the subsystem as holistically. So, generally. when you, I've already done that for you, kind of. You have, like, one buddy is one subsystem by itself, so most of the time you can do your whole part by yourself. But once you start integrating them, then you can get better resolution, better quality. Sometimes you need to start working with your members to… for holding to work, for your whole part to work, but generally you can work independently, then later on starts

Combining them, like, maybe two buddies or 3 buddies at a time, until finally all the buddy works… can work all together. Okay. I'll give… One more thing, let me assure you that the Pico is powerful enough to handle all of this. It has been done. it can be done. So, don't tell me, sir, impossible, Pico not powerful enough to do all this. Actually, PICO can do more than this, actually. The capabilities is ridiculous. So it can do more than this, so don't be, that should be the last thing in the minor, at least. So again, the different…

body responsibilities, I put there, clearly, what you need to do. These are some other, just examples, you don't have to follow per se. So, but you need to think about it, okay?

### Alternative projects and closing

I think that is all. Okay, so that is all for that default project. What about the non-default project? Sorry, that's my mouse. So the non-default project, there's a few of them. Okay, number one is… But this… another one is that it's configurable Okay. I will not, I think, go into one by one. If you are interested in any one of those alternative projects. approach me directly, I will explain to you what it does, okay? And when you approach me, please open up your laptop with the description of it so that I can quickly recall what is it about, and I can explain to you.

But again, these are just… general description of the various alternative projects. So, for the robotica, it's very clear what you need to do. So you can go ahead with it, and design it, and come up with the requirements. But for the alternative projects, I'm only giving you generally what it should do. So there are… there are certain areas where you can choose what… how you want to pivot the project. So you can make a decision there. But again, if you are interested in any one of the alternative projects.

find me during the lab, I can… I'll be more than happy to explain to you, and then you can decide whether you want to go with it or not. Okay? Don't ask me, sir, explain for me all of them. It can be quite overwhelming. So, so read it through, do your own, search. then come to me, once you have very specific questions, you know? Okay? Then I will explain to you, you know, what I think, and if you don't agree and you want to do it somewhere else, then I'm okay with that.

By all means. Okay? Okay, so we'll go with that. Again, all the descriptions are there, and you can, by all means, ask me questions in a discussion forum if you have any doubt. If you didn't have time to meet me during the lab, then you can always ask me during a discussion forum. That tends to happen quite a lot, actually. So, with that, I think thank you very much. And I'll see you in the lab.
