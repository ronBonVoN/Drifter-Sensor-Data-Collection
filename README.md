This is Arduino code I am writing as a Research Assistant at the University of Southern Maine. 
It will read data from sensors on an off-shore drifter bouey meant to collect data of Maine waters. 
Sensor reading code has already been estabilished, 
the previous goal of this code was to conserve power by turning sensors off when not reading data using a relay, 
and putting the Arduino into a low-power sleep mode at the same time. 
Now reliable power saving code has been established. The current goal is to add on the ability to remotly check
drifter status by sending data over cellular network to a discord webhook. Then have a discord bot organize the sent data. 
