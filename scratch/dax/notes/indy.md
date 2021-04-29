# Key Assumptions
1. Gaia is responsible for high level policy decisions and not local loop control.
1. Gaia must make all decisions much less that 20 ms (for room).
1. Pain point around logging.  Can Gaia help here either via tables or its spd logging?
    * Probably not - separate ROS2 node
    * Z1 Analyser/Z1 server?
1. Most likely taking on roll of Race Path Controller and command Race Vehicle Controller?
1. Global strategy and tactics?

# RVC Notes and Spec
## Policies
1. Tire warmup
1. Racing
1. Flag policies
1. Race line instructions based on occupancy grid
1. IAC Race Control Messages and Logic

## Race Vehicle Controller
### Inputs
1. Input is a set of Race lines.
    * refinements to race line?
1. Vehicle parameters/dynamic info
1. Per Cycle updates
    * Every 20 Hz, get Vehicle Path Trajectory update from Race Path Controller
    * Also gets actions and constraints from RPC.
    * Engine RPM
1. Race Control and Team Control also send inputs.

### Outputs
1. Update controllers - resultant steering, throttle, and brake values
1. 

### RVC Controllers?
1. Corners:  Traction Circle Controller (TCC)
1. Straights
    * Launch System Controller (LSC)
    * Anti-Brake System - mirror image of LSC
1. RPM Speed Limiter Controller
1. Line Transition Controller (LTC)
    * Takes Race Path Controller commands to change position (GPS lat/long, heading & speed)
    * Creates Transition Curvature Path:  compute steering, throttle, brake commands.
    * LTC continues current TCP until RPC overrides with new instructions
    * Vehicle dynamics may override exact RPC command speeds
1. Emergency Action Controller - loses comms
1. Race Line Optimizer (RLO)
    * Could be a set of rules but fatest if just C function with math.
    * called on every Vehicle Path Trajectory Update message (VPTU)
    * Target Speed history is important.

### Questions
1. Interaction between RPC and RVC.  Which "level" does Gaia operate at?  Guessing RPC.
1. Both RPC and RVC do logging.  Intentional, correct?

# UH-IAC Vehicle Control System

## Race Path Controller (in SCADE now, but wants to move off)
1. Race control state machine (trackes where vehicle is along GPS waypoint array)
1. Perception operator - detects "bogies"
1. Steering Control operator
1. Speed control operator

## Race Vehicle Controller (now in C++)
1. Handles vehicle dynamics and the limits they impose.

## Messages
1. TTL Init message, 
    * 32 TTLs
    * 999 waypoints
    * 22 positions
Might look like:
```
struct waypoint {
    int64_t positions[22];
};


struct TTL {
    const char* name;
    waypoint* waypoints[999];
}

TTL* ttls[32];
```
2. Secondary logic table will be useful (front-straight 1, start-finish, front-straight 2, ... )
1.  Trajectory Cycle Update Message (this is an update to the trajectory, not commands?)
    * 

1. TTL arrays are read only?  Are these paths

# Simulator
ANSYS VRXperience Simulator (also known as VRX)

# 2/11/21:  Raw Notes
ANSYS - provides simulator
street car simulator - vehicle dynamics, has a lot of the right pieces in a new part of the simulator


Goal:  Develop an autonomous software stack
Like Robo Car.  Look at YouTube.  Leading autonomous vehicle racing program that has been out there.

Rationale help

Pose, speed, and heading

vehicle controller
path controller

Heavily lean on GPS

track edge
dynamic vehicle identification
slow moving/static within two track boundaries

sh.htl file -> data structures and streams from VRX simulator, that is specified
polynomial for track edges, heading and speed

Exact data versus simulated data.

Sim, Win10 (VRX)
- four DDS messages/schemas
- DONE
- RTI:  subscribe to, read info about the vehicle, detected vehicles, DDS interface (ask Mark), 
- RTI: write DDS into simulator
- WAIT: simulator and software are cycle locked (determinisic).  DDS -> ROS2 compliant,

Hamachi VPN.
- Sync method of waits and dones
- 20 Hz - 50 Hz.
- 20 Hz -> 50 ms
- 50 Hz -> 20 ms
- 100Hz -> 10 ms

Path versus heading, time to conflict.
Solve for time to conflict.
Assign bogies to quadrants.  Develop rules for bogies in quadrants.

Race cars:  offensive or defensive, or in another condition
Run a path prediction system (look ahead 3s)
Bogie on optimum race line, likely to remain on race line

Strategic rules versus constraints (occupancy grid)

Race Path controller
- strategic racing rules
- 

Server -> ROS node 

Use gaia to build bogy classifier, path controller, flesh out pseudo-design docs
architecture document

Say that again:
- straight away fixed speed
- around all four turns
- see how long it took

Put everything in a VM

Chassis Sim.

Send over a message that says:  
- stored way point path you are on (optimum race line one)
- send override, path controller - move to another race path, with a time to accomplish that (1 or 2sec)
- using your smarts about the vehicle dynamics, etc.

Zoom sooner than Thursday

# 2/16/2021 notes
"Think and act" should only be ROS messages.
Our only input is ROS messages
-> update the database
Our only output is ROS messages
<- rules, ROS messages, possibly write directly to our own

Protoype would be good based on their SCADE-less solution
Replace one rule with gaia
hand back

# 2/17/2021 notes
# 2/18/2021 notes
Daniel(?) is point guy on that.  V2 form.
spec with appendix over to us
next week, thin rpc (with new RVC), running around middle of track
- build system
- ansys simulator (ms teams site), training files, relevant, messages, how to utilize the simulator
- ROS2 -> VRXperience, ROS2 -> Hamachi -> ROS2 Bridge -> DDX -> VRX Simulator, 3 VM accounts (one of those accounts)
- Access 24/7 to simulator, build and test against that.  Compartmentalized.  Access to 2 linux VM so that we can run development version of RPC with dev version of RVC.  Linux VMs (to connect to win10 machine to run to simulator).
- Simulatorr -> single exe for RPC and RVC.  Current RPC and RVC broken into three pieces.  RPC is master of sync to simulator (BLC, RPC, RVC) -> Autonomus Vehicle System.  Ansys through Azure - each VM.  
- Follow up with Gary (on hardware)
- Daniel - doing dev on private PC, using Hamachi to connect back into VPN of simulation.  
- Finds location of EGO, determins next GPS waypoint, sends message to RVC go to that waypoint, tell simulator to do its next cycle.  Really, really thin.  Maybe not a lot of rules.
- Next set of rules:  Go to fastest race line rules:  where are you?  is fastest line to left or right of me?  If left or right then tell RVC to start moving from current TTL to other TTL.  Then tracks that transition until overlaps entirely and we've moved.  Maybe you are 5 lanes away from fastest race line.  Four steps away.  TTL == Target Trajectory Line
- Mark:  express interest in getting a definitive list of data in and the data we need to provide.  All ROS messages?  Great.  If not, we need to get there at some point.
- Gary:  VRX messages and data structures inside those messages are inherent in Sh.html and network.html that he sent us.  Those data structures are accessed by ROS2 messages that are programmed and available in GitHub where ROS2 -> VRX and VRX -> ROS2 to apps.  That subscribe/publisher boundary has been built by others.  Become a shared github that everyone has access to.  That GitHub is what Daniel is using to create messages to send/receive from simulator.  Thin RPC gets the okay to go from simulator.  Thin RPC reads ROS (subscribe to message inside VRX - either in sh or network. html.  Sh is vehicle controller message.  Network is perception system messages).  Either read by thin RPC or RVC as needed.  RVC writes back to simulator all vehicle control messages (steering, breaking, accelerator).  Sends message back to RPC saying RVC is done.  RPC then sends "done" message to Simulator and then simulator can go on to next cycle.  Messages between RPC and RVC are described in overview document. Subject to review "reality check".  Evolution of messages sending back and forth internally.  Assume we wouldn't use interprocess but we would use intraprocess or shared memory, etc.  (Gaia Db should go here!).
- Mark:  should architect our model to be abstracted from process model in *terms* of ROS2 interaction.  My (Dax) point is only that from the gaia perspective our rules and db in the same process is a big advantage. And, we don't have a model where db writes in a separate process can invoke rules in another process (at this point).  
- Gary:  BLC is a "bridge" product.  Camera provides bounding boxes, headings, title, speed and not just pixels.  Gives us all this for each identified bogies.  All the "hard work" of identifying, classifying, that is all behind the scenes.  The camera is "perfect".  And the simulator knows all the locations/etc up front. BLC starts with that.  Lidars and radars are closer to real-world stuff (but still, no noise, and exist in a perfect world and never miss a beat.  All that simplifies all that code - can be done in a deterministic way and not in a probalistic way.  Everything is also in sync.  All of that comes in (see network.html) - will be subscribed to by BLC when it is its turn to do its job. BLC - grab list from simulator, go through and assign them to occupancy grid (hand-waving), which will drive the decision logic on whether we should slow down, speed up, move left, move right.  All done in BLC location/classification level.  Above that is another level for emergency purposes.  If vehicles aren't operating within reasonable boundaries of how vehicles work, then there is no way our normal rules will work so we have to go to emergency-level rules.  Can we solve 80% of problems in emergency mode?  That's good enough.  Keep rules straight forward when "everyone" crashes.  BLC turns over classified and oriented grid as long as who is on that grid that we care about.  Only care about vehicles that are within a temporal range of distance and time that we care about and then let RPC integrate that with yellow flags, team telling car to come into the pits and other external things that affect what the vehicle is doing the next cycle.
- Zach:  so this is more in the realm of data fusion than data processing.  Sounds less complicated than I had assumed it would be.  
- Mark:  More work and naming system in occupancy grid?
- Gary:  If bogey is in 2 then bogey is in from of us.  We have to unwind track in front of us so that somebody is right in front of us if we are in the middle of the turn yet is actually on our left.  Have to see if someone is *actually* in our lane (bogey is in a separate part of the track than we are).  We keep unwinding short because the car may be too far from us for it to matter (like 3-5 seconds).
- Mark:  we are not taking in any video.  Our code doesn't change via simulator versus real-world.
- Gary:  pick someone in your org (Me!)  To figure out how we want to establish project/modules or not.  How we want to handle software source control, etc.  Presumption - lot of evolution in RVC (freeze, let Daniel go around).  Try to define messages and protocols between the two.
- Daniel: in Romania.  This is his life right now.  
- Daniel/Wade/Alex:  three people we communicate with.  New slack channel in their system and invite you guys (Gaia).  Alex is TTL guy.  Wade is RPC guy.  Wade will be consultant/advisor on lower-levels.  Not from programming perspective but from a description method.

