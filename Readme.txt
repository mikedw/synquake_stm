Mandatory Parameters:
N_THREADS   = number of server threads used for processing clients actions
            = between 1 and MAX_THREADS (defined in src/server/server.h)
N_CLIENTS   = number of simulated clients
N_CYCLES    = number of simulated server frames
N_QUESTS    = number of quest sessions, distributed evenly in time across the N_CYCLES
QSpread     = number of quest locations associated with each quest session
QuestsFile  = file containing the quests locations for all the quest sessions
            = format of the quests file (on each line) :
               quest_session_id quest_location_id quest_position_x quest_position_y
            = quest_session_id and quest_location_id start from 0
            = if N_QUESTS is given as a negative number then the coordinates of each
              quest_location( quest_position_x, quest_position_y ) is interpreted as
              a fractional number out of the size of the map. This is useful when
              wanting to test the same quest configuration with maps of different sizes.
            = for an example check this file: 4quadrants.quest 
MapSizeX    = length of the map
MapSizeY    = width of the map
TreeDepth   = the depth of the area_node tree used for managing objects on the map
SpeedMax    = the maximum speed of a client. SpeedMin is set to SpeedMax/2
AppleRatio  = the ratio( out of 1000 ) of the map that is covered by Apples
WallRatio   = the ratio( out of 1000 ) of the map that is covered by Walls
BalanceType = the name of the policy used for load balancing
            = available options: none static1 static2 lightest spread quadtree areatree
ActionsFile = file describing the pattern of actions to be executed by each client
              within one server frame. If the file is not found, it uses the default
              pattern: 1 move per server frame.
              For some examples and more info on writing these files check :
              straight_moves.conf and random_actions.conf

Optional Parameters:
print      = how frequent in percentages to print stats about entities distribution
             and thread assignment
           = if "print" is given as a negative number then it will also dump with the
             same frequency the position of each player in a file called
             players_%date%_%time%.out, that can later be fed as a parameter to
             tools/plot_players.scr for some nice printouts.

lock_type  = type of locking algorithms used. Options: lock_tree (default) and
             lock_leaves.
 
