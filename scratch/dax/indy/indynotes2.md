# Indy Notes 2

# Questions
* I see the TimeToCollision input but don't see when the heading will collide.  (i.e., how to determine we'll intersect?)
* Asked Wade this question.  Need for emergency maneuvers
* VPTU message:  What is the Waypoint_Index.  Where I am or where I want to go?
- from wade - TTL where I want to go, same as waypoint
* Trans_to_2 logic seems to imply that I need to know the number of bogies in the core quadrant.  I thought I only need to know whether there was 1 or 0.

* more help with Trans to 2 logic (is fastest OTL always Lane 4 OTL), fastest MTL is always Lane 4 MTL, fasted HTL is awways lane 4 HTL??  From Wade:  Fastest OTL?  Tire degradation module will take care of.  Fastest OTL takes into account tire stickiness (.85 to .65).  Move to fastest OTL?  Fastest OTL will change over the race.  Hierarchy will be in an array that can be changed.


* in Trans_to_2 logic:  what is QFE (Simplified)
* in Trans_to_2 logic:  what is follow case (follow who?)  also in 2 bogies case:  is there a type?

	If 2 bogies in Q1 & Q2 OR 2 bogies in Q2 & Q3 AND can Catch both
		If in OTL move to OTL3
		if in MTL move to MTL3
			if in HTL move to HTL3
	Else
		If in OTL move to fastest OTL and Follow
		if in MTL move to fastest MTL and Follow
			if in HTL move to fastest HTL and Follow
* in Second tactic.  Stay in current TTL and Follow (who
* in Second tactic, when we want to allow a bogie to move from Q4 to Q1, how much should we slow?
* in Second tactic, is the distance supposed to be in Y(?)

* First tactic, Follow whom?  (Maybe just TTL line?)  When do we set the RLO_Flag?
* Emergency vehicle array:  why does the emergency vehcile array have 6 slots instead of 4? (Q1, Q2, Q3, Q9)

Wade notes 4/13/21

EMERGENCY vehicla manuevering stuff:  equation to intersect
slots should mirror the same six slots we use for Segmented (for the emergency vehilce array)

From Wade:  X is forward and Y is left and right!

positive X is in front, negative X is rear
positive Y is left, negative Y is right

# New Questions
* TTL_Trans_Dist how do I know this distance?
* keep previous cycle wayline delta?  (Currently not doing that!)
-1 means go as fast as you can.


# Internal notes
* would be good to have support for read-only data (i.e., no transaction necessary?)
* tunable parameters are interesting
* transcaction overhead?

