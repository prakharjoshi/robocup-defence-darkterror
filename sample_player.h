// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef SAMPLE_PLAYER_H
#define SAMPLE_PLAYER_H

#include "action_generator.h"
#include "field_evaluator.h"
#include "communication.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <vector>
 #include "bhv_basic_offensive_kick.h"
#include "strategy.h"

#include "bhv_basic_tackle.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include "neck_offensive_intercept_neck.h"

#include <string>
#include <vector>

class SamplePlayer
    : public rcsc::PlayerAgent {
private:

    Communication::Ptr M_communication;

    FieldEvaluator::ConstPtr M_field_evaluator;
    ActionGenerator::ConstPtr M_action_generator;

public:

    SamplePlayer();

    virtual
    ~SamplePlayer();

protected:

    /*!
      You can override this method.
      But you must call PlayerAgent::initImpl() in this method.
    */
    virtual
    bool initImpl( rcsc::CmdLineParser & cmd_parser );

    //! main decision
    virtual
    void actionImpl();

    //! communication decision
    virtual
    void communicationImpl();

    virtual
    void handleActionStart();
    virtual
    void handleActionEnd();

    virtual
    void handleServerParam();
    virtual
    void handlePlayerParam();
    virtual
    void handlePlayerType();

    virtual
    FieldEvaluator::ConstPtr createFieldEvaluator() const;

    virtual
    ActionGenerator::ConstPtr createActionGenerator() const;

public:

    bool mpIntransit;
    rcsc::Vector2D mpTarget;
    bool IsOccupying;
    rcsc::Vector2D PrevOccupied;

    bool Opponenthasball;

    std::string lastRole;

    int
    LastBH;

    bool
    FoundNewBH;


    bool 
    executeSampleRole( rcsc::PlayerAgent * agent );

    bool
    PlacePlayers(rcsc::PlayerAgent * agent);

    bool
    BasicMove( rcsc::PlayerAgent * agent );

    bool 
    isKickable(rcsc::PlayerAgent * agent, int unum);

    int 
    GetBHUnum(rcsc::PlayerAgent * agent);

    double 
    abs(double d);

    std::vector < rcsc::Vector2D > calculatePlacementThreat();

    rcsc::Vector2D 
    calculate_placement_threat(rcsc::PlayerAgent * agent, rcsc::Vector2D P);

    rcsc::Vector2D 
    RoundToNearestTens(rcsc::Vector2D P);
    
    int
    ClosestPlayerToBall(rcsc::PlayerAgent * agent);

    bool 
    isRTaHole(rcsc::Vector2D P);

    rcsc::Vector2D 
    RoundToNearestHole(rcsc::Vector2D P);

    bool
    AreSamePoints(rcsc::Vector2D A, rcsc::Vector2D B, double buffer);

    bool
    IsOccupied(rcsc::PlayerAgent * agent, rcsc::Vector2D target, double buffer);

    bool
    IsOccupiedWhileDashing(rcsc::PlayerAgent * agent, rcsc::Vector2D target, double buffer);

    int
    IsOccupiedForPassing(rcsc::PlayerAgent * agent, rcsc::Vector2D target, double buffer);


    int
    GetOccupierUnum(rcsc::PlayerAgent * agent, rcsc::Vector2D target, double buffer);

    bool
    PassToBestPlayer(rcsc::PlayerAgent * agent);

    bool
    PassToPlayer( rcsc::PlayerAgent * agent, rcsc::Vector2D target_point, int receiver );

    bool
    DecideAndOccupyHole(rcsc::PlayerAgent * agent, int target);

    void
    OccupyHole(rcsc::Vector2D target);

    bool
    PassPlayersAvailable(rcsc::PlayerAgent * agent);

    bool
    AreSameNos(double A, double B, double buffer);

    bool
    IsSectorEmpty(rcsc::PlayerAgent * agent, rcsc::Vector2D target, double buf_degree);

    bool
    SampleDribble(rcsc::PlayerAgent * agent);


    bool
    ManMark( rcsc::PlayerAgent * agent );

    bool
    ManMark2( rcsc::PlayerAgent * agent );
    
    bool 
    sendCentreBack( rcsc::PlayerAgent * agent );

    bool
    executeDefense( rcsc::PlayerAgent * agent );


private:
    double getDashPower( const rcsc::PlayerAgent * agent );
    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
    bool doPreprocess();
    bool doShoot();
    bool doForceKick();
    bool doHeardPassReceive();

public:
    virtual
    FieldEvaluator::ConstPtr getFieldEvaluator() const;
};

#endif
