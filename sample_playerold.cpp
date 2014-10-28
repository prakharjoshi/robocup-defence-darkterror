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
 along with this code; see the mfile COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

//bin/rione_player -team Ri-one -f conf/formations.conf -c conf/player.conf -num 1


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sample_player.h"

#include "strategy.h"
#include "field_analyzer.h"

#include "action_chain_holder.h"
#include "sample_field_evaluator.h"

#include "soccer_role.h"

#include "sample_communication.h"
#include "keepaway_communication.h"

#include "bhv_penalty_kick.h"
#include "bhv_set_play.h"
#include "bhv_set_play_kick_in.h"
#include "bhv_set_play_indirect_free_kick.h"
#include "bhv_basic_move.h" 

#include "bhv_custom_before_kick_off.h"
#include "bhv_strict_check_shoot.h"

#include "view_tactical.h"

#include "intention_receive.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_emergency.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/body_smart_kick.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_point.h> 
#include <rcsc/action/view_synch.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/body_dribble.h>

#include <rcsc/formation/formation.h>
#include <rcsc/action/kick_table.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/audio_sensor.h>
#include <rcsc/player/freeform_parser.h>
#include "bhv_chain_action.h"
#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/body_dribble.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/body_pass.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/common/basic_client.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>
// #include <rcsc/common/free_message_parser.h>

#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>

#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector> 

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
SamplePlayer::SamplePlayer()
    : PlayerAgent(),
      M_communication(),
      M_field_evaluator( createFieldEvaluator() ),
      M_action_generator( createActionGenerator() )
{
    LastBH = -1;
    FoundNewBH = false;
    mpIntransit = false;
    IsOccupying = false;
    Opponenthasball = false;
    lastRole = "undecided";
    mpTarget = rcsc::Vector2D(0,0);


    boost::shared_ptr< AudioMemory > audio_memory( new AudioMemory );

    M_worldmodel.setAudioMemory( audio_memory );

    //
    // set communication message parser
    //
    addSayMessageParser( SayMessageParser::Ptr( new BallMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new PassMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new InterceptMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new GoalieMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new GoalieAndPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new OffsideLineMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new DefenseLineMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new WaitRequestMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new PassRequestMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new DribbleMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new BallGoalieMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new OnePlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new TwoPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new ThreePlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new SelfMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new TeammateMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new OpponentMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new BallPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new StaminaMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new RecoveryMessageParser( audio_memory ) ) );

    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 9 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 8 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 7 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 6 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 5 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 4 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 3 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 2 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 1 >( audio_memory ) ) );

    //
    // set freeform message parser
    //
    setFreeformParser( FreeformParser::Ptr( new FreeformParser( M_worldmodel ) ) );

    //
    // set action generators
    //
    // M_action_generators.push_back( ActionGenerator::Ptr( new PassGenerator() ) );

    //
    // set communication planner
    //
    M_communication = Communication::Ptr( new SampleCommunication() );
}

/*-------------------------------------------------------------------*/
/*!

 */
SamplePlayer::~SamplePlayer()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SamplePlayer::initImpl( CmdLineParser & cmd_parser )
{
    bool result = PlayerAgent::initImpl( cmd_parser );

    // read additional options
    result &= Strategy::instance().init( cmd_parser );

    rcsc::ParamMap my_params( "Additional options" );
#if 0
    std::string param_file_path = "params";
    param_map.add()
        ( "param-file", "", &param_file_path, "specified parameter file" );
#endif

    cmd_parser.parse( my_params );

    if ( cmd_parser.count( "help" ) > 0 )
    {
        my_params.printHelp( std::cout );
        return false;
    }

    if ( cmd_parser.failed() )
    {
        std::cerr << "player: ***WARNING*** detected unsuppprted options: ";
        cmd_parser.print( std::cerr );
        std::cerr << std::endl;
    }

    if ( ! result )
    {
        return false;
    }

    if ( ! Strategy::instance().read( config().configDir() ) )
    {
        std::cerr << "***ERROR*** Failed to read team strategy." << std::endl;
        return false;
    }

    if ( KickTable::instance().read( config().configDir() + "/kick-table" ) )
    {
        std::cerr << "Loaded the kick table: ["
                  << config().configDir() << "/kick-table]"
                  << std::endl;
    }

    return true;
}

    //FallenBackOnce = false;
    //fallBackCount = 0;

/*-------------------------------------------------------------------*/
/*!
  main decision
  virtual method in super class
*/
void
SamplePlayer::actionImpl()
{
    //
    // update strategy and analyzer
    //
    Strategy::instance().update( world() );
    FieldAnalyzer::instance().update( world() );

    //
    // prepare action chain
    //
    M_field_evaluator = createFieldEvaluator();
    M_action_generator = createActionGenerator();

    ActionChainHolder::instance().setFieldEvaluator( M_field_evaluator );
    ActionChainHolder::instance().setActionGenerator( M_action_generator );

    //
    // special situations (tackle, objects accuracy, intention...)
    //
    const WorldModel & wm = this->world();

    if ( doPreprocess() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": preprocess done" );
        return;
    }

    //
    // update action chain
    //
    ActionChainHolder::instance().update( world() );

    //
    // create current role
    //
    std::string role_name;
    SoccerRole::Ptr role_ptr;
    {
        role_ptr = Strategy::i().createRole( world().self().unum(), world() );
        role_name = Strategy::i().getRoleName( world().self().unum(), world() );

        if ( ! role_ptr )
        {
            std::cerr << config().teamName() << ": "
                      << world().self().unum()
                      << " Error. Role is not registerd.\nExit ..."
                      << std::endl;
            M_client->setServerAlive( false );
            return;
        }
    }

    //
    // override execute if role accept
    //
    if ( role_ptr->acceptExecution( world() ) )
    {
        if(role_name=="Sample"){
            executeSampleRole(this);
            return;
        }
        role_ptr->execute( this );
        return;
    }

    //
    // play_on mode
    //
    if ( world().gameMode().type() == GameMode::PlayOn )
    {
        if(role_name=="Sample"){
            executeSampleRole(this);
            return;
        }
        role_ptr->execute( this );
        return;
    }

    if ( world().gameMode().type() == GameMode::KickOff_)
    {   
        mpIntransit = false;
        if(wm.self().unum()==10){
            mpIntransit = true;
            mpTarget = wm.ball().pos();
        }
        bool kickable = this->world().self().isKickable();
        if ( this->world().existKickableTeammate()
             && this->world().teammatesFromBall().front()->distFromBall()
             < this->world().ball().distFromSelf() )
        {
            kickable = false;
        }

        if ( kickable )
        {
            if(!PassToBestPlayer( this )){
                Body_HoldBall().execute( this );
                //doKick( this);
                //Bhv_BasicOffensiveKick().execute(this);
                //PassToBestPlayer(this);   
            }                
        }
        else
        {
            doMove(this);
        }
        return;
    }

    //
    // penalty kick mode
    //
    if ( world().gameMode().isPenaltyKickMode() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": penalty kick" );
        Bhv_PenaltyKick().execute( this );
        return;
    }

    //
    // other set play mode
    //
    Bhv_SetPlay().execute( this );
    return;

    if(role_name=="Sample"){
        executeSampleRole(this);
        return;
    }
}



/*--------------------------------------------------------------------------*/

bool
SamplePlayer::SampleDribble( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const PlayerPtrCont & opps = wm.opponentsFromSelf();
    const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
    const Vector2D nearest_opp_pos = ( nearest_opp
                                       ? nearest_opp->pos()
                                       : Vector2D( -1000.0, 0.0 ) );

    Vector2D pass_point;

    if ( Body_Pass::get_best_pass( wm, &pass_point, NULL, NULL ) )
    {
        if ( pass_point.x > wm.self().pos().x - 1.0 )
        {
            bool safety = true;
            const PlayerPtrCont::const_iterator opps_end = opps.end();
            for ( PlayerPtrCont::const_iterator it = opps.begin();
                  it != opps_end;
                  ++it )
            {
                if ( (*it)->pos().dist( pass_point ) < 4.0 )
                {
                    safety = false;
                }
            }

            if ( safety )
            {
                Body_Pass().execute( agent );
                agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
                return true;
            }
        }
    }

    if ( nearest_opp_dist < 3.0 )
    {
        if ( Body_Pass().execute( agent ) )
        {
            agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
            return true;
        }
    }

    // dribble to my body dir
    if ( nearest_opp_dist < 5.0
         && nearest_opp_dist > ( ServerParam::i().tackleDist()
                                 + ServerParam::i().defaultPlayerSpeedMax() * 1.5 )
         && wm.self().body().abs() < 70.0 )
    {
        const Vector2D body_dir_drib_target
            = wm.self().pos()
            + Vector2D::polar2vector(5.0, wm.self().body());

        int max_dir_count = 0;
        wm.dirRangeCount( wm.self().body(), 20.0, &max_dir_count, NULL, NULL );

        if ( body_dir_drib_target.x < ServerParam::i().pitchHalfLength() - 1.0
             && body_dir_drib_target.absY() < ServerParam::i().pitchHalfWidth() - 1.0
             && max_dir_count < 3
             )
        {
            // check opponents
            // 10m, +-30 degree
            const Sector2D sector( wm.self().pos(),
                                   0.5, 10.0,
                                   wm.self().body() - 30.0,
                                   wm.self().body() + 30.0 );
            // opponent check with goalie
            if ( ! wm.existOpponentIn( sector, 10, true ) )
            {
                Body_Dribble( body_dir_drib_target,
                              1.0,
                              ServerParam::i().maxDashPower(),
                              2
                              ).execute( agent );
                agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
                return true;
            }
        }
    }

    Vector2D drib_target( 50.0, wm.self().pos().absY() );
    if ( drib_target.y < 20.0 ) drib_target.y = 20.0;
    if ( drib_target.y > 29.0 ) drib_target.y = 27.0;
    if ( wm.self().pos().y < 0.0 ) drib_target.y *= -1.0;
    const AngleDeg drib_angle = ( drib_target - wm.self().pos() ).th();

    // opponent is behind of me
    if ( nearest_opp_pos.x < wm.self().pos().x + 1.0 )
    {
        // check opponents
        // 15m, +-30 degree
        const Sector2D sector( wm.self().pos(),
                               0.5, 15.0,
                               drib_angle - 30.0,
                               drib_angle + 30.0 );
        // opponent check with goalie
        if ( ! wm.existOpponentIn( sector, 10, true ) )
        {
            const int max_dash_step
                = wm.self().playerType()
                .cyclesToReachDistance( wm.self().pos().dist( drib_target ) );
            if ( wm.self().pos().x > 35.0 )
            {
                drib_target.y *= ( 10.0 / drib_target.absY() );
            }
            Body_Dribble( drib_target,
                          1.0,
                          ServerParam::i().maxDashPower(),
                          std::min( 5, max_dash_step )
                          ).execute( agent );
        }
        else
        {
            Body_Dribble( drib_target,
                          1.0,
                          ServerParam::i().maxDashPower(),
                          2
                          ).execute( agent );

        }
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    // opp is far from me
    if ( nearest_opp_dist > 2.5 )
    {
        Body_Dribble( drib_target,
                      1.0,
                      ServerParam::i().maxDashPower(),
                      1
                      ).execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    // opp is near

    if ( Body_Pass().execute( agent ) )
    {
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }


    // opp is far from me
    if ( nearest_opp_dist > 3.0 )
    {
        Body_Dribble( drib_target,
                      1.0,
                      ServerParam::i().maxDashPower(),
                      1
                      ).execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    /*
    if ( nearest_opp_dist > 2.5 )
    {
        Body_HoldBall().execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }
    */

    {
        Body_AdvanceBall().execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
    }

    return true;

}

bool 
SamplePlayer::PassPlayersAvailable( PlayerAgent * agent ){
    const WorldModel & wm = agent->world();

    Vector2D myPosition = wm.self().pos();
    Vector2D currentHole = RoundToNearestHole(myPosition);
    Vector2D frontup = Vector2D(currentHole.x+10, currentHole.y-10);
    Vector2D backup = Vector2D(currentHole.x-10, currentHole.y-10);
    Vector2D frontdown = Vector2D(currentHole.x+10, currentHole.y+10);
    Vector2D backdown = Vector2D(currentHole.x-10, currentHole.y+10);

    Vector2D fronthor = Vector2D(currentHole.x+20, currentHole.y);
    Vector2D backhor = Vector2D(currentHole.x-20, currentHole.y);
    Vector2D upvert = Vector2D(currentHole.x, currentHole.y-20);
    Vector2D downvert = Vector2D(currentHole.x, currentHole.y+20);
    
    double buffer = 2.5;
    
    //TODO: Return true only if pass is advantageous
    
    if( IsOccupiedForPassing(agent, frontup, buffer)||
        IsOccupiedForPassing(agent, frontdown, buffer)||
        IsOccupiedForPassing(agent, backup, buffer)||
        IsOccupiedForPassing(agent, backdown, buffer)||
        IsOccupiedForPassing(agent, fronthor, buffer)||
        IsOccupiedForPassing(agent, upvert, buffer)||
        IsOccupiedForPassing(agent, downvert, buffer)||
        IsOccupiedForPassing(agent, backhor, buffer)
        ){
        return true;
    }

    /*
    const WorldModel & world = agent->world();
    const PlayerPtrCont::const_iterator 
    t_end = world.teammatesFromSelf().end();
    for ( PlayerPtrCont::const_iterator
              it = world.teammatesFromSelf().begin();
          it != t_end;
          ++it )
    {
        if(((*it)->pos().dist(world.ball().pos()))<=20.0){
            return false;
        }  
    }
    */
    
    return false;   
}

bool
SamplePlayer::IsSectorEmpty(PlayerAgent * agent, Vector2D target, double buf_degree){
    Neck_TurnToPoint( target ).execute(agent );
    const WorldModel & wm = agent->world();
    AngleDeg target_angle = (target - wm.self().pos()).dir();
    double target_dis = wm.self().pos().dist(target);
    const Sector2D sector( wm.self().pos(),
                            0.5, target_dis + 4,
                            target_angle - buf_degree,
                            target_angle + buf_degree );
    // opponent check without goalie
    if ( ! wm.existOpponentIn( sector, 10, false ) )
    {
        return true;
    }

    return false;
}

bool
SamplePlayer::PassToBestPlayer( PlayerAgent * agent ){
    //std::cout<<"****************************************************************"<<std::endl;
    //Bhv_BasicOffensiveKick().execute(agent);
    //return true;
    const WorldModel & wm = agent->world();

    Vector2D myPosition = wm.self().pos();
    Vector2D currentHole = RoundToNearestHole(myPosition);

    Vector2D frontup = Vector2D(currentHole.x+10, currentHole.y-10);
    Vector2D backup = Vector2D(currentHole.x-10, currentHole.y-10);
    Vector2D frontdown = Vector2D(currentHole.x+10, currentHole.y+10);
    Vector2D backdown = Vector2D(currentHole.x-10, currentHole.y+10);

    Vector2D fronthor = Vector2D(currentHole.x+20, currentHole.y);
    Vector2D backhor = Vector2D(currentHole.x-20, currentHole.y);
    Vector2D upvert = Vector2D(currentHole.x, currentHole.y-20);
    Vector2D downvert = Vector2D(currentHole.x, currentHole.y+20);
    
    double buffer = 2.5;
    double buf_degree = 25;

    if(IsOccupiedForPassing(agent, frontup, buffer) && IsSectorEmpty(agent, frontup, buf_degree))
        return PassToPlayer(agent, frontup, IsOccupiedForPassing(agent, frontup, buffer));
    if(IsOccupiedForPassing(agent, frontdown, buffer) && IsSectorEmpty(agent, frontdown, buf_degree))
        return PassToPlayer(agent, frontdown, IsOccupiedForPassing(agent, frontdown, buffer));

    
    if(IsOccupiedForPassing(agent, fronthor, buffer) && IsSectorEmpty(agent, fronthor, buf_degree))
        return PassToPlayer(agent, fronthor, IsOccupiedForPassing(agent, fronthor, buffer));
    if(IsOccupiedForPassing(agent, upvert, buffer) && IsSectorEmpty(agent, upvert, buf_degree))
        return PassToPlayer(agent, upvert, IsOccupiedForPassing(agent, upvert, buffer));
   
    if(IsOccupiedForPassing(agent, downvert, buffer) && IsSectorEmpty(agent, downvert, buf_degree))
        return PassToPlayer(agent, downvert, IsOccupiedForPassing(agent, downvert, buffer));
    
    if(IsOccupiedForPassing(agent, backup, buffer) && IsSectorEmpty(agent, backup, buf_degree))
        return PassToPlayer(agent, backup, IsOccupiedForPassing(agent, backup, buffer));
    if(IsOccupiedForPassing(agent, backdown, buffer) && IsSectorEmpty(agent, backdown, buf_degree))
        return PassToPlayer(agent, backdown, IsOccupiedForPassing(agent, backdown, buffer));


    if(IsOccupiedForPassing(agent, backhor, buffer) && IsSectorEmpty(agent, backhor, buf_degree))
        return PassToPlayer(agent, backhor, IsOccupiedForPassing(agent, backhor, buffer));
    
    return false;
}

int
SamplePlayer::ClosestPlayerToBall(PlayerAgent * agent){
    double mindis = 999;
    int mindisunum = -1;
    for(int i=2; i<=11; i++){
        if(agent->world().ourPlayer(i)!=NULL){
            if(agent->world().ourPlayer(i)->distFromBall() < mindis){
                mindis = agent->world().ourPlayer(i)->distFromBall();
                mindisunum = i;
            }
        }
    }
    return mindisunum;
}


bool
SamplePlayer::sendCentreBack(PlayerAgent * agent) {
    const WorldModel & wm = agent->world();
    int uniformNumber = wm.self().unum();
    Vector2D ballPosition = wm.ball().pos();
    Vector2D myPosition = wm.self().pos();
    Vector2D destinationPointCentre = Vector2D( -45.5, 0 );
    Vector2D destinationPointUpper = Vector2D( -47.5, 6 );
    Vector2D destinationPointDown = Vector2D( -47.5, -6 );
    if (ballPosition.x < 15 && myPosition.x > -47.5) { 
        
            



            //Intercept the ball (if it is the better option) instead of running for your point of defense
            


            /*
            const int self_min = wm.interceptTable()->selfReachCycle();
            const int mate_min = wm.interceptTable()->teammateReachCycle();
            const int opp_min = wm.interceptTable()->opponentReachCycle();

            //Intercept
            if ( ! wm.existKickableTeammate()
                 && ( self_min <= 3
                      || ( self_min <= mate_min
                           && self_min < opp_min + 3 )
                      )
                 )
            {
                std::cout<<"body intercept called for player - "<<wm.self().unum()<<std::endl;

            
                dlog.addText( Logger::TEAM,
                              __FILE__": intercept");
                Body_Intercept().execute( agent );
                agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
            }    
            */

            const PlayerObject * nearest_opponent = wm.getOpponentNearestToBall( 10 );
             if ( nearest_opponent && ( nearest_opponent->distFromBall() > wm.self().distFromBall()) )
            {
                Body_Intercept().execute( agent );
                agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
            }
        //    else {       
                //else dash toward your point of defense
            

            

            if(ballPosition.y < 0) {
                Body_GoToPoint(destinationPointUpper, 0.5, ServerParam::i().maxDashPower()).execute( agent );
            } else if (ballPosition.y > 0) {
                Body_GoToPoint(destinationPointDown, 0.5, ServerParam::i().maxDashPower()).execute( agent );
            } else {
                Body_GoToPoint(destinationPointCentre, 0.5, ServerParam::i().maxDashPower()).execute( agent );                
            }

            

            //Body_GoToPoint(Vector2D(ballPosition.x - 10, 0), 0.5, ServerParam::i().maxDashPower()).execute( agent );        
          //  }

        if (AreSamePoints(myPosition, ballPosition, 15)) {
        //if ((abs(ballPosition.x - myPosition.x) < 10) && (abs(ballPosition.y - myPosition.y) < 10)) {  
            //Body_TurnToBall().execute( agent );
            //Body_GoToPoint(ballPosition, 0.5, ServerParam::i().maxDashPower()).execute( agent );
            //Bhv_BasicTackle( 0.8, 80.0 ).execute( agent );

            //Body_Intercept().execute( agent );
            //agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        
           //     Body_TurnToBall().execute( agent );
           //     Body_Intercept().execute( agent );
           //     agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
            

            //Bhv_BasicMove().execute(agent);

            
            
            if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
            {   
                std::cout<<"body intercept called for player - "<<wm.self().unum()<<std::endl;

            
                dlog.addText( Logger::TEAM,
                              __FILE__": intercept");
                Body_Intercept().execute( agent );
                agent->setNeckAction( new Neck_OffensiveInterceptNeck() );   
            }
            
            
            /*--------------------------------------------------------*/
            // chase ball
            /*
            const int self_min = wm.interceptTable()->selfReachCycle();
            const int mate_min = wm.interceptTable()->teammateReachCycle();
            const int opp_min = wm.interceptTable()->opponentReachCycle();

            //Intercept
            if ( ! wm.existKickableTeammate()
                 && ( self_min <= 3
                      || ( self_min <= mate_min
                           && self_min < opp_min + 3 )
                      )
                 )
            {
                std::cout<<"body intercept called for player - "<<wm.self().unum()<<std::endl;

            
                dlog.addText( Logger::TEAM,
                              __FILE__": intercept");
                Body_Intercept().execute( agent );
                agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
                return true;
            }
            */



       }

    } else {
        Bhv_BasicMove().execute(agent);
    }
    return true;
}

bool
SamplePlayer::ManMark2( PlayerAgent * agent ){

//     const WorldModel & wm = agent->world();
    
//     //-----------------------------------------------
//     //tackle
    
//     if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
//     {   
//         return true;
//     }

//     /*--------------------------------------------------------*/
//     // chase ball
//     const int self_min = wm.interceptTable()->selfReachCycle();
//     const int mate_min = wm.interceptTable()->teammateReachCycle();
//     const int opp_min = wm.interceptTable()->opponentReachCycle();

//     //Intercept
//     if ( ! wm.existKickableTeammate()
//          && ( self_min <= 3
//               || ( self_min <= mate_min
//                    && self_min < opp_min + 3 )
//               )
//          )
//     {
//         std::cout<<"body intercept called for player - "<<wm.self().unum()<<std::endl;

    
//         dlog.addText( Logger::TEAM,
//                       __FILE__": intercept");
//         Body_Intercept().execute( agent );
//         agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
//         return true;
//     }



//    //Bhv_BasicMove().execute(agent);



    
//     const PlayerPtrCont & opps = wm.opponentsFromSelf();
//     const PlayerObject * nearest_opp
//         = ( opps.empty()
//             ? static_cast< PlayerObject * >( 0 )
//             : opps.front() );
//     const double nearest_opp_dist = ( nearest_opp
//                                       ? nearest_opp->distFromSelf()
//                                       : 1000.0 );
//     const Vector2D nearest_opp_pos = ( nearest_opp
//                                        ? nearest_opp->pos()
//                                        : Vector2D( -1000.0, 0.0 ) );

//     Vector2D to_mark_position ;
//     Vector2D ball_chasers_point;
//     Vector2D upper_dee_spot = Vector2D(-47 ,5);

//     Vector2D lower_dee_spot = Vector2D(-47,-5);

//     Vector2D ball_pos = wm.ball().pos(); 



//     if (wm.self().pos().y>0){
//         ball_chasers_point = upper_dee_spot;
//     }
//     else{
//         ball_chasers_point = lower_dee_spot;
//     }


//     int ball_pos_y = ball_pos.y;
//     int y_displacement = 0.5;
//     int x_displacement = 2;

//     if (ball_pos.x>10){
//         x_displacement = 1;
//         }
//     else if (ball_pos.x>-35){
//         x_displacement = 6;
//     }
//     else {
//         x_displacement = 3.5;
//         y_displacement = 2;
//         }

    
//    // to_mark_position.x = nearest_opp_pos.x - x_displacement;
//     int x_disp_dir = ball_pos.x - wm.self().pos().x;
//     if (x_disp_dir>0){
//         x_disp_dir = 1;
//     }
//     else{
//         x_disp_dir = -1;
//     }



//     x_disp_dir = -1;
//     x_displacement = x_displacement*x_disp_dir;


    


//     if (to_mark_position.x < -52 ){
//         to_mark_position.x = -52;
//     }

//     int y_disp_dir = ball_pos_y - wm.self().pos().y;
//     if (y_disp_dir>0){
//         y_disp_dir = 1;
//     }
//     else{
//         y_disp_dir = -1;
//     }

//     y_disp_dir = wm.self().pos().y/abs(wm.self().pos().y)*(-1);

//     y_displacement = y_displacement*y_disp_dir;
    

//     to_mark_position.y = nearest_opp_pos.y + y_displacement;
//     to_mark_position.x = nearest_opp_pos.x + x_displacement;

//     int kompany;
//     int ballchaser;
//     //Searching for Kompany
//     if((agent->world().self().pos().x - 1) < wm.ourDefenseLineX()) 
//             kompany = agent->world().self().unum();
     
//         if(wm.self().unum() == kompany) {
//             sendCentreBack(agent);
//         }
//     kompany = 9999;

//     //Ensuring one-to-one marking
//     int mydistfromopp = nearest_opp_pos.dist(wm.self().pos());

//     const PlayerObject * opp_nearest_to_ball = wm.getOpponentNearestToBall( 10 );
//     if ((nearest_opp == opp_nearest_to_ball)&&(nearest_opp_pos.x<wm.self().pos().x)){
//         if(wm.self().pos().x>ball_chasers_point.x)
//             ballchaser = agent->world().self().unum();
//     }



//     int mindis = mydistfromopp;
//     int mindisunum = -1;
//     bool goodtogo = false;



//     for(int i=2; i<=11; i++){
//         if((agent->world().ourPlayer(i)!=NULL)&&(i!=wm.self().unum())) {

//             if(agent->world().ourPlayer(i)->pos().dist(nearest_opp_pos) < mindis){
//                 if(i!=kompany){
//                 Vector2D ppos = agent->world().ourPlayer(i)->pos();
//                 if((ppos.dist(nearest_opp_pos) < mindis) && (ppos.x<wm.self().pos().x) ){

//                 goodtogo = false;
//                 break;
//             }
//                // mindis = agent->world().ourPlayer(i)->distFromBall();

//                // mindisunum = i;
//             }
//             }
//             else{
//                 ;
//             }
//         }
//         goodtogo = true;
//     }
//     if (wm.self().unum() == kompany){
//         goodtogo = false ;
//     }

//     if (goodtogo){
//         if (wm.self().unum() == ballchaser){
//         Body_GoToPoint(ball_chasers_point, 0.5, ServerParam::i().maxDashPower()).execute( agent );
//         }
//         else{

//          Body_GoToPoint(to_mark_position, 0.5, ServerParam::i().maxDashPower()).execute( agent );
//          //Bhv_BasicTackle( 0.8, 80.0 ).execute( agent );
//         }
//     }
//     else{
//         //If didn't have anyone to mark
//         //Body_GoToPoint(upper_dee_spot, 0.5, ServerParam::i().maxDashPower()).execute( agent );
//        // sendCentreBack(agent);
//         if (wm.self().unum() != kompany) {

//         Bhv_BasicMove().execute(agent);
//         }

//     }

//     return true;
    
// }
    const WorldModel & wm = agent->world();
    const double dash_power = Strategy::get_normal_dash_power( wm );
    
    //-----------------------------------------------
    // tackle
    
    if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
    {   
        return true;
    }

    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    //Intercept
    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + 3 )
              )
         )
    {
        std::cout<<"body intercept called for player - "<<wm.self().unum()<<std::endl;

    
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept");
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        return true;
    }







    
    const PlayerPtrCont & opps = wm.opponentsFromSelf();
    const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
    const Vector2D nearest_opp_pos = ( nearest_opp
                                       ? nearest_opp->pos()
                                       : Vector2D( -1000.0, 0.0 ) );

    Vector2D to_mark_position ;
    Vector2D ball_chasers_point;
    Vector2D upper_dee_spot = Vector2D(-47 ,5);
    Vector2D fallback = Vector2D(-50 ,0);

    Vector2D lower_dee_spot = Vector2D(-47,-5);

    Vector2D ball_pos = wm.ball().pos(); 



    if (wm.self().pos().y>0){
        ball_chasers_point = upper_dee_spot;
    }
    else{
        ball_chasers_point = lower_dee_spot;
    }


    int ball_pos_y = ball_pos.y;
    int y_displacement = 0.5;
    int x_displacement = 2;

    if (ball_pos.x>10){
        x_displacement = 1;
        }
    else if (ball_pos.x>-35){
        x_displacement = 6;
    }
    else {
        x_displacement = 3.5;
        y_displacement = 2;
        }

    
   // to_mark_position.x = nearest_opp_pos.x - x_displacement;
    int x_disp_dir = ball_pos.x - wm.self().pos().x;
    if (x_disp_dir>0){
        x_disp_dir = 1;
    }
    else{
        x_disp_dir = -1;
    }



    x_disp_dir = -1;
    x_displacement = x_displacement*x_disp_dir;


    


    if (to_mark_position.x < -52 ){
        to_mark_position.x = -52;
    }

    int y_disp_dir = ball_pos_y - wm.self().pos().y;
    if (y_disp_dir>0){
        y_disp_dir = 1;
    }
    else{
        y_disp_dir = -1;
    }

    y_disp_dir = wm.self().pos().y/abs(wm.self().pos().y)*(-1);

    y_displacement = y_displacement*y_disp_dir;
    

    to_mark_position.y = nearest_opp_pos.y + y_displacement;
    to_mark_position.x = nearest_opp_pos.x + x_displacement;

    int kompany;
    int ballchaser;
    //Searching for Kompany
    if((agent->world().self().pos().x - 1) < wm.ourDefenseLineX()) 
            kompany = agent->world().self().unum();
     
        if(wm.self().unum() == kompany) {
            sendCentreBack(agent);
        }
    kompany = 9999;

    //Ensuring one-to-one marking
    int mydistfromopp = nearest_opp_pos.dist(wm.self().pos());

    const PlayerObject * opp_nearest_to_ball = wm.getOpponentNearestToBall( 10 );
    if ((nearest_opp == opp_nearest_to_ball)&&(nearest_opp_pos.x<wm.self().pos().x)){
        if(wm.self().pos().x>ball_chasers_point.x)
            ballchaser = agent->world().self().unum();
    }



    int mindis = mydistfromopp;
    int mindisunum = -1;
    bool goodtogo = false;



    for(int i=2; i<=11; i++){
        if((agent->world().ourPlayer(i)!=NULL)&&(i!=wm.self().unum())) {

            if(agent->world().ourPlayer(i)->pos().dist(nearest_opp_pos) < mindis){
                if(i!=kompany){
                Vector2D ppos = agent->world().ourPlayer(i)->pos();
                if((ppos.dist(nearest_opp_pos) < mindis) && (ppos.x<wm.self().pos().x) ){

                goodtogo = false;
                break;
            }
               // mindis = agent->world().ourPlayer(i)->distFromBall();

               // mindisunum = i;
            }
            }
            else{
                ;
            }
        }
        goodtogo = true;
    }
    if (wm.self().unum() == kompany){
        goodtogo = false ;
    }

    if (goodtogo){
        if (wm.self().unum() == ballchaser){
        Body_GoToPoint(ball_chasers_point, 0.5, ServerParam::i().maxDashPower()).execute( agent );
        }
        else{
             double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    
    
    if ( ! Body_GoToPoint( to_mark_position, dist_thr, dash_power
                           ).execute( agent ) )
    {
        Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {

      /*
        If the ball is with opponent team then intercept the ball.
        Accordingly team with the shortest distance will approach for ball.
      */
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
    }


         //Body_GoToPoint(to_mark_position, 0.5, ServerParam::i().maxDashPower()).execute( agent );
         //Bhv_BasicTackle( 0.8, 80.0 ).execute( agent );
        }
    }
    else{
        //If didn't have anyone to mark
        //Body_GoToPoint(fallback, 0.5, ServerParam::i().maxDashPower()).execute( agent );
       // sendCentreBack(agent);
        if (wm.self().unum() != kompany) {
            sendCentreBack(agent);

        //Bhv_BasicMove().execute(agent);
        }

    }

    return true;
    
}








//DEFENSE FUNCTION
bool
SamplePlayer::ManMark( PlayerAgent * agent ){
    const WorldModel & wm = agent->world();
    const PlayerPtrCont & opps = wm.opponentsFromSelf();
    const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
    const Vector2D nearest_opp_pos = ( nearest_opp
                                       ? nearest_opp->pos()
                                       : Vector2D( -1000.0, 0.0 ) );

    Vector2D to_mark_position ;
    Vector2D upper_dee_spot = Vector2D(-47,11);

    Vector2D lower_dee_spot = Vector2D(-47,11);

    Vector2D ball_pos = wm.ball().pos(); 
    int ball_pos_y = ball_pos.y;
    int y_displacement = 0;
    int x_displacement = 0;
    if (ball_pos.x>10){
        int x_displacement = 0.5;
        }
    else if (ball_pos.x>-20){
        int x_displacement = 6;
    }
    else {
        int x_displacement = 1;
        y_displacement = 2;
        }
   // to_mark_position.x = nearest_opp_pos.x - x_displacement;
    int x_disp_dir = ball_pos.x - wm.self().pos().x;
    if (x_disp_dir>0){
        x_disp_dir = 1;
    }
    else{
        x_disp_dir = -1;
    }

    //overriding
    x_disp_dir = -1;

    x_displacement = x_displacement*x_disp_dir;


    to_mark_position.x = nearest_opp_pos.x + x_displacement;


    if (to_mark_position.x < -52 ){
        to_mark_position.x = -52;
    }

    int y_disp_dir = ball_pos_y - wm.self().pos().y;
    if (y_disp_dir>0){
        y_disp_dir = 1;
    }
    else{
        y_disp_dir = -1;
    }

    y_displacement = y_displacement*y_disp_dir;


    to_mark_position.y = nearest_opp_pos.y + y_displacement;

    
    //Ensuring one-to-one marking
    int mydistfromopp = nearest_opp_pos.dist(wm.self().pos());
    int mindis = mydistfromopp;
    int mindisunum = -1;
    bool goodtogo = true;

    Body_GoToPoint(to_mark_position, 0.5, ServerParam::i().maxDashPower()).execute( agent );
    Bhv_BasicTackle( 0.8, 80.0 ).execute( agent );
    for(int i=2; i<=11; i++){
        if((agent->world().ourPlayer(i)!=NULL)&&(i!=wm.self().unum())) {
            Vector2D ppos = agent->world().ourPlayer(i)->pos();
            if((ppos.dist(nearest_opp_pos) < mindis) && (ppos.x<wm.self().pos().x) ){
                goodtogo = false;
                break;
               // mindis = agent->world().ourPlayer(i)->distFromBall();

               // mindisunum = i;
            }
            else{
                ;
            }
        }
        goodtogo = true;
    }


    if (!goodtogo){
        Vector2D spos = wm.self().pos();
        //spos.x = spos.x - 2;

       // Body_GoToPoint(spos, 0.5, ServerParam::i().maxDashPower()).execute( agent );
    
        if(!Bhv_BasicTackle( 0.8, 80.0 ).execute( agent )){
        
    
    
    
    
    
    
    /*--------------------------------------------------------*/
    // chase ball
        if (wm.ball().pos().dist(wm.self().pos())<3){
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
    }
    Body_GoToPoint(spos, 0.5, ServerParam::i().maxDashPower()).execute( agent );
    
}

    bool upper_dee_taken = false;
    bool lower_dee_taken = false;
//Logic for the players who are free in 1/4 of the ground, will run to the goal and try to stand in front of the goal
/*if (wm.self().pos().x<-10){
    for(int i=2; i<=11; i++){
        if((agent->world().ourPlayer(i)!=NULL)&&(i!=wm.self().unum())) {

            if(AreSamePoints( agent->world().ourPlayer(i)->pos(), upper_dee_spot, 2 )) {
                
                upper_dee_taken = true;
               // mindis = agent->world().ourPlayer(i)->distFromBall();

               // mindisunum = i;
            }
        }
        

    }
    if (!upper_dee_taken){
        Body_GoToPoint(upper_dee_spot, 0.5, ServerParam::i().maxDashPower()).execute( agent );
    }
    else{
        for(int i=2; i<=11; i++){
        if((agent->world().ourPlayer(i)!=NULL)&&(i!=wm.self().unum())) {

            if(AreSamePoints( agent->world().ourPlayer(i)->pos(), lower_dee_spot, 2 )) {
                
                lower_dee_taken = true;
               // mindis = agent->world().ourPlayer(i)->distFromBall();

               // mindisunum = i;
            }
        }
        

    }
    if (!lower_dee_taken){
        Body_GoToPoint(lower_dee_spot, 0.5, ServerParam::i().maxDashPower()).execute( agent );
    }
    else{
        //still free
    }
    }
    */
    int mydisfromupper = wm.self().pos().dist(upper_dee_spot);
    int mydisfromlower = wm.self().pos().dist(lower_dee_spot);
    if ((wm.self().pos().x<-10)&&(wm.ball().pos().x<-10)){
        if (wm.ball().pos().y<0){
                Body_GoToPoint(lower_dee_spot, 0.5, ServerParam::i().maxDashPower()).execute( agent );
            if(!Bhv_BasicTackle( 0.8, 80.0 ).execute( agent )){
                if (wm.ball().pos().dist(wm.self().pos())<5){
                    Body_Intercept().execute( agent );
                    agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
                    }
                }
        }
        else{
            Body_GoToPoint(upper_dee_spot, 0.5, ServerParam::i().maxDashPower()).execute( agent );
            if(!Bhv_BasicTackle( 0.8, 80.0 ).execute( agent )){
                if (wm.ball().pos().dist(wm.self().pos())<5){
                    Body_Intercept().execute( agent );
                    agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
                    }
                }
        }






        /*
    for(int i=2; i<=11; i++){
        if((agent->world().ourPlayer(i)!=NULL)&&(i!=wm.self().unum())) {

            if(agent->world().ourPlayer(i)->pos().dist(upper_dee_spot) < mydisfromupper){
                upper_dee_taken = true;
                break;
               // mindis = agent->world().ourPlayer(i)->distFromBall();

               // mindisunum = i;
            }
            else{
                ;
            }
        }
        upper_dee_taken = false;
    }

    if (!upper_dee_taken){
        //goingtopen = true;
        Body_GoToPoint(upper_dee_spot, 0.5, ServerParam::i().maxDashPower()).execute( agent );
        if(!Bhv_BasicTackle( 0.8, 80.0 ).execute( agent )){
        if (wm.ball().pos().dist(wm.self().pos())<5){
            Body_Intercept().execute( agent );
            agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        }
}
    

    }
    else{
      for(int i=2; i<=11; i++){
        if((agent->world().ourPlayer(i)!=NULL)&&(i!=wm.self().unum())) {

            if((agent->world().ourPlayer(i)->pos().dist(lower_dee_spot) < mydisfromlower)&&(!AreSamePoints(agent->world().ourPlayer(i)->pos(),upper_dee_spot,1))){
                lower_dee_taken = true;
                break;
               // mindis = agent->world().ourPlayer(i)->distFromBall();

               // mindisunum = i;
            }
            else{
                ;
            }
        }
        lower_dee_taken = false;
    }

    if (!lower_dee_taken){
        //goingtopen = true;
        Body_GoToPoint(lower_dee_spot, 0.5, ServerParam::i().maxDashPower()).execute( agent );
        if(!Bhv_BasicTackle( 0.8, 80.0 ).execute( agent )){
        
    
    
    
    
    */
    
    /*--------------------------------------------------------
    //chase ball
        if (wm.ball().pos().dist( wm.self().pos() ) <5){
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
    }
*/
}

    

     
    else{
        //still free
    }


 }

    return true;
    
}

bool
SamplePlayer::executeDefense( PlayerAgent * agent ){
dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove" );

    //-----------------------------------------------
    // tackle
    if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
    {
        return true;
    }

    const WorldModel & wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + 3 )
              )
         )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept" );
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );

        return true;
    }


    // here you get the position

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    const double dash_power = Strategy::get_normal_dash_power( wm );

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

          dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f",
                  target_point.x, target_point.y,
                  dist_thr );

    agent->debugClient().addMessage( "BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );
    
    
    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power
                           ).execute( agent ) )
    {
        Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {

      /*
        If the ball is with opponent team then intercept the ball.
        Accordingly team with the shortest distance will approach for ball.
      */
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
    }

    return true;
}











bool
SamplePlayer::executeSampleRole( PlayerAgent * agent )
{   
    if(agent->config().teamName()=="opp"){
        Body_GoToPoint( Vector2D(-50,0), 0.0, ServerParam::i().maxDashPower(), -1, 4, true, 60).execute( agent );
        return true;
    }
    bool kickable = agent->world().self().isKickable();
    if ( agent->world().existKickableTeammate()
         && agent->world().teammatesFromBall().front()->distFromBall()
         < agent->world().ball().distFromSelf() )
    {
        kickable = false;
    }

    if(agent->world().existKickableTeammate())
        Opponenthasball = false;
    else{};
        //Opponenthasball = true;
    if(agent->world().existKickableOpponent()){
        Opponenthasball = true;
        if(agent->world().self().unum()==ClosestPlayerToBall(agent)){
            Body_GoToPoint(agent->world().ball().pos(), 0.5, ServerParam::i().maxDashPower()).execute( agent );
            return true;
        }
    }

    if ( kickable )
    {
        //doKick( this);
        //return true;
        
        if(!PassToBestPlayer( this )){
            //SampleDribble(this);
            //Body_HoldBall().execute( this );
            doKick( this);
            //Bhv_BasicOffensiveKick().execute(this);
            //PassToBestPlayer(this);   
        }
                       
    }
    else
    /*{   //DEFENSE MECHHANISM CALL
        if(Opponenthasball)
           // doMove(agent);
           { 

             //sendCentreBack(agent);
             if(agent->world().ball().pos().x > -10 )
                ManMark2(agent);
             else
                Bhv_BasicMove().execute(agent);          //MSHL
           }
        else{
            if(agent->world().ball().pos().x > -25 )
                   doMove(agent);
            else 
                Bhv_BasicMove().execute(agent); 
            //ManMark2(agent);/**/
           // doMove( agent );
    //    }
    //}
    {

     
        if(Opponenthasball || agent->world().ball().pos().x > 25 || agent->world().self().pos().x > 25)
            Bhv_BasicMove().execute(agent);
        else{
            if((agent->world().self().unum()==2 || agent->world().self().unum()==3 ))
                Bhv_BasicMove().execute(agent);
            else{
                if (agent->world().ball().pos().x < -40) 
                    doMove( agent );
                else
                    Bhv_BasicMove(agent);


            }
        }
    } 


    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SamplePlayer::doKick( PlayerAgent * agent )
{
    
    if ( Bhv_ChainAction().execute( agent ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (execute) do chain action" );
        agent->debugClient().addMessage( "ChainAction" );
        return;
    }

    Bhv_BasicOffensiveKick().execute( agent );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SamplePlayer::doMove( PlayerAgent * agent )
{
        BasicMove(agent);
}

bool
SamplePlayer::BasicMove(PlayerAgent * agent){
    const WorldModel & wm = agent->world();
    
    //-----------------------------------------------
    // tackle
    
    if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
    {   
        return true;
    }

    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    //Intercept
    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + 3 )
              )
         )
    {
        std::cout<<"body intercept called for player - "<<wm.self().unum()<<std::endl;

    
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept");
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        return true;
    }
    
    //Check if ball has been passed
    /*
    if(wm.existKickableTeammate()){
        int CurrentBH = GetBHUnum(agent);
        if(CurrentBH!=LastBH && CurrentBH!=-1){
            FoundNewBH = true;
            LastBH = CurrentBH;
        }
        else
            FoundNewBH = false;

        //Opponenthasball=false;
    }
    */
    
    const Vector2D target_point = Vector2D(0,0);
    const double dash_power = ServerParam::i().maxDashPower();

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    if(mpIntransit && !FoundNewBH){

        int occupier_unum = IsOccupiedForPassing(agent, mpTarget, 7);

        if(occupier_unum > 0){
            OccupyHole(PrevOccupied);
        }

        else if(AreSamePoints(mpTarget, wm.self().pos(), 0.1)){
            mpIntransit = false;
        }
                
        else {
            Body_GoToPoint( mpTarget, 0.0, dash_power, -1, 4, true, 60).execute( agent );
            return true; 
        }
    }

    //Decide and Occupy hole
    if( true ){
        if( ! DecideAndOccupyHole( agent, -1) )
        {
            agent->setNeckAction( new Neck_TurnToBallOrScan() );
        }
        return true;
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
    }

    return true;
}

bool
SamplePlayer::PassToPlayer( PlayerAgent * agent, Vector2D target_point, int receiver )
{
    if ( ! agent->world().self().isKickable() || receiver < 1)
    {
        return false;
    }

    /*
    if ( Bhv_ChainAction().execute( agent ) )
    {
        return true;
    }
    */

    double first_speed = ServerParam::i().ballSpeedMax()/1.1;

    // evaluation
    //   judge situation
    //   decide max kick step

    agent->debugClient().addMessage( "pass" );
    agent->debugClient().setTarget( target_point );

    int kick_step = ( agent->world().gameMode().type() != GameMode::PlayOn
                      && agent->world().gameMode().type() != GameMode::GoalKick_
                      ? 1
                      : 3 );
    if ( ! Body_SmartKick( target_point,
                           first_speed,
                           first_speed * 0.96,
                           kick_step ).execute( agent ) )
    {
        if ( agent->world().gameMode().type() != GameMode::PlayOn
             && agent->world().gameMode().type() != GameMode::GoalKick_ )
        {
            first_speed = std::min( agent->world().self().kickRate() * ServerParam::i().maxPower(),
                                    first_speed );
            Body_KickOneStep( target_point,
                              first_speed
                              ).execute( agent );
        }
        else
        {
            return false;
        }
    }

    if ( agent->config().useCommunication()
         && receiver != Unum_Unknown )
    {
        Vector2D target_buf = target_point - agent->world().self().pos();
        target_buf.setLength( 1.0 );

        agent->addSayMessage( new PassMessage( receiver,
                                               target_point + target_buf,
                                               agent->effector().queuedNextBallPos(),
                                               agent->effector().queuedNextBallVel() ) );
    }

    return true;
}

bool 
SamplePlayer::DecideAndOccupyHole(PlayerAgent * agent, int target){
    //Called when another teammate would have the ball
    //target == -1 means a player already has the ball
    
    const WorldModel & wm = agent->world();
    Vector2D MyPos = wm.self().pos();
    MyPos = RoundToNearestHole(MyPos);

    /*
    if(!mpIntransit){
        if(!AreSamePoints(MyPos, wm.self().pos(), 0.1)){
            OccupyHole(MyPos);
            return true;
        }
    }*/

    Vector2D BallPos = wm.ball().pos();
    BallPos = RoundToNearestHole(BallPos);
    PrevOccupied = MyPos;
    Vector2D BHPos;
            
    if(target==-1){
        BHPos = wm.ball().pos(); //Since the the ballholder already has the ball
    }
    else if(target!=wm.self().unum()){
        BHPos = wm.ourPlayer(target)->pos();
    }
    else{
        return false;
    }
    
    BHPos = RoundToNearestHole(BHPos);
    std::cout<<"Ball position - player "<<wm.self().unum()<<" "<<BHPos<<std::endl;
    std::cout<<"My position - player "<<wm.self().unum()<<" "<<MyPos<<std::endl;    

    //BHPos and BallPos are used interchangeably

    Vector2D BHfrontup = Vector2D(BHPos.x+10, BHPos.y-10);
    Vector2D BHbackup = Vector2D(BHPos.x-10, BHPos.y-10);
    Vector2D BHfrontdown = Vector2D(BHPos.x+10, BHPos.y+10);
    Vector2D BHbackdown = Vector2D(BHPos.x-10, BHPos.y+10);

    Vector2D BHfronthor = Vector2D(BHPos.x+20, BHPos.y);
    Vector2D BHupfronthor = Vector2D(BHPos.x+20, BHPos.y-20);
    Vector2D BHdownfronthor = Vector2D(BHPos.x+20, BHPos.y+20);

    Vector2D BHbackhor = Vector2D(BHPos.x-20, BHPos.y);
    Vector2D BHupbackhor = Vector2D(BHPos.x-20, BHPos.y-20);
    Vector2D BHdownbackhor = Vector2D(BHPos.x-20, BHPos.y+20);
    
    Vector2D BHupvert = Vector2D(BHPos.x, BHPos.y-20);
    Vector2D BHdownvert = Vector2D(BHPos.x, BHPos.y+20);

    double obuffer = 1.5;
    double cbuffer = 1.5;

    bool BallInEvenLine;
    bool BallAboveMidLine;
    bool BallOnMidline;

    int BPLine = static_cast<int>(abs(BHPos.x+5)/10);
    
    if( BPLine%2 == 0 )
        BallInEvenLine = false;
    else
        BallInEvenLine = true;

    if(AreSameNos(0, BallPos.y, obuffer)){
        BallOnMidline = true;
        BallAboveMidLine = false;
    }
    else if(BallPos.y < 0){
        BallAboveMidLine = true;
        BallOnMidline = false;
    }
    else{
        BallAboveMidLine = false;
        BallOnMidline = false;
    }
    
    //Ball in even level
    if(BallInEvenLine){
        //Ball above midline
        if(BallAboveMidLine){
            //Level 0
            if(AreSameNos(BallPos.x, MyPos.x, obuffer)){
                if(AreSamePoints(MyPos, BHupvert, obuffer)){
                    if(!IsOccupied(this, BHfrontup, cbuffer)){
                        OccupyHole(BHfrontup);
                        return true;
                    }
                }
                else if(AreSamePoints(MyPos, BHdownvert, obuffer)){
                    if(!IsOccupied(this, BHfrontdown, cbuffer)){
                        OccupyHole(BHfrontdown);
                        return true;
                    }
                }
                return true;
            }

            //Level 1
            else if(AreSameNos(BallPos.x - 10, MyPos.x, obuffer)){
                if(AreSamePoints(MyPos, BHbackup, obuffer)){
                    if(!IsOccupied(this, BHupvert, cbuffer)){
                        OccupyHole(BHupvert);
                        return true;
                    }
                }
                else if(AreSamePoints(MyPos, BHbackdown, obuffer)){
                    if(!IsOccupied(this, BHdownvert, cbuffer)){
                        //OccupyHole(BHdownvert);
                        return true;
                    }
                }
                else{
                    if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y-10), cbuffer)){
                        OccupyHole(Vector2D(MyPos.x+10, MyPos.y-10));
                        return true;
                    }
                }
            }
            //All other levels behind the ball holder
            else if(MyPos.x < BallPos.x - 15)
            {
                if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y-10), cbuffer)){
                    OccupyHole(Vector2D(MyPos.x+10, MyPos.y-10));
                    return true;
                }
            }
            //Level -1
            else if(AreSameNos(BallPos.x+10, MyPos.x, obuffer))
                return true;
            //Level -2 onwards
            else if(MyPos.x > BallPos.x + 15){
                if(AreSamePoints(Vector2D(MyPos.x-10, MyPos.y-10), BHfrontup, obuffer))
                    return true;
                if(AreSamePoints(Vector2D(MyPos.x-10, MyPos.y-10), BHfrontdown, obuffer))
                    return true;
                //Level -2 does not move, since conflict with Level 0 players at Level -1
                //if(AreSameNos(MyPos.x - 20, BallPos.x, obuffer))
                  //  return true;
                if(!IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y-10), cbuffer)){
                    OccupyHole(Vector2D(MyPos.x-10, MyPos.y-10));
                    return true;
                }
            }
        }
        //Ball below midline
        else{
            //Level 0
            if(AreSameNos(BallPos.x, MyPos.x, obuffer)){
                if(AreSamePoints(MyPos, BHupvert, obuffer)){
                    if(!IsOccupied(this, BHfrontup, cbuffer)){
                        OccupyHole(BHfrontup);
                        return true;
                    }
                }
                else if(AreSamePoints(MyPos, BHdownvert, obuffer)){
                    if(!IsOccupied(this, BHfrontdown, cbuffer)){
                        OccupyHole(BHfrontdown);
                        return true;
                    }
                }
                return true;
            }
            //Level 1
            else if(AreSameNos(BallPos.x - 10, MyPos.x, obuffer)){
                if(AreSamePoints(MyPos, BHbackup, obuffer)){
                    if(!IsOccupied(this, BHupvert, cbuffer)){
                        //OccupyHole(BHupvert);
                        return true;
                    }
                }
                else if(AreSamePoints(MyPos, BHbackdown, obuffer)){
                    if(!IsOccupied(this, BHdownvert, cbuffer)){
                        OccupyHole(BHdownvert);
                        return true;
                    }
                }
                else{
                    if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y+10), cbuffer)){
                        OccupyHole(Vector2D(MyPos.x+10, MyPos.y+10));
                        return true;
                    }
                }
            }
            //All other levels behind the ball holder
            else if(MyPos.x < BallPos.x - 15)
            {
                if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y+10), cbuffer)){
                    OccupyHole(Vector2D(MyPos.x+10, MyPos.y+10));
                    return true;
                }
            }
            //Level -1
            else if(AreSameNos(BallPos.x+10, MyPos.x, obuffer))
                return true;
            //Level -2 onwards
            else if(MyPos.x > BallPos.x + 15){
                if(AreSamePoints(Vector2D(MyPos.x-10, MyPos.y+10), BHfrontup, obuffer))
                    return true;
                if(AreSamePoints(Vector2D(MyPos.x-10, MyPos.y+10), BHfrontdown, obuffer))
                    return true;
                //Level -2 does not move, since conflict with Level 0 players at Level -1
                //if(AreSameNos(MyPos.x - 20, BallPos.x, obuffer))
                  //  return true;
                if(!IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y+10), cbuffer)){
                    OccupyHole(Vector2D(MyPos.x-10, MyPos.y+10));
                    return true;
                }
            }
        }
    }
    //Ball in odd level
    else{
        //Ball on midline
        if(BallOnMidline){
            int MyLine = static_cast<int>((abs(MyPos.x)+5)/10);
            bool MeInEvenLine;
    
            if( MyLine%2 == 0 )
                MeInEvenLine = false;
            else
                MeInEvenLine = true;

            //Level 0
            if(AreSameNos(BHPos.x, MyPos.x, obuffer)){
                
                //Level 0 - above midline, ball on midline
                if(MyPos.y<0){
                    if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y+10), cbuffer)){
                        OccupyHole(Vector2D(MyPos.x+10, MyPos.y+10));
                        return true;
                    }
                }
                //Level 0 - below midline, ball on midline
                else{
                    if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y-10), cbuffer)){
                        OccupyHole(Vector2D(MyPos.x+10, MyPos.y-10));
                        return true;
                    }
                }
            }
            //Behind the ball
            else if(MyPos.x < BHPos.x){
                //Even levels behind the ball
                if(MeInEvenLine){     
                    if(MyPos.y<0){
                        if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y-10), cbuffer)){
                            OccupyHole(Vector2D(MyPos.x+10, MyPos.y-10));
                            return true;
                        }
                    }
                    else{
                        if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y+10), cbuffer)){
                            OccupyHole(Vector2D(MyPos.x+10, MyPos.y+10));
                            return true;
                        }
                    }
                }
                //Odd levels behind the ball
                else{
                    //if player on midline
                    if(AreSameNos(0, MyPos.y, obuffer)){
                        if(!IsOccupied(this, Vector2D(MyPos.x, MyPos.y-20), cbuffer)
                            &&
                           !IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y-10), cbuffer))
                        {
                            //OccupyHole(Vector2D(MyPos.x+10, MyPos.y-10));
                            return true;
                        }
                        else if(!IsOccupied(this, Vector2D(MyPos.x, MyPos.y+20), cbuffer)
                            &&
                           !IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y+10), cbuffer))
                        {
                            //OccupyHole(Vector2D(MyPos.x+10, MyPos.y+10));
                            return true;
                        }

                    }
                    else if(MyPos.y<0){
                        if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y+10), cbuffer)){
                            OccupyHole(Vector2D(MyPos.x+10, MyPos.y+10));
                            return true;
                        }
                    }
                    else if(MyPos.y >0){
                        if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y-10), cbuffer)){
                            OccupyHole(Vector2D(MyPos.x+10, MyPos.y-10));
                            return true;
                        }
                    }
                }
            }
            //Ahead of the ball + 15
            else if (MyPos.x > BHPos.x + 15){
                //Even levels ahead of ball
                if(MeInEvenLine){
                    if(MyPos.y<0){
                        if(!IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y-10), cbuffer)){
                            OccupyHole(Vector2D(MyPos.x-10, MyPos.y-10));
                            return true;
                        }
                    }
                    else{
                        if(!IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y+10), cbuffer)){
                            OccupyHole(Vector2D(MyPos.x-10, MyPos.y+10));
                            return true;
                        }
                    }
                }
                //Odd levels ahead of ball
                else{
                    //if on midline
                    if(AreSameNos(0, MyPos.y, obuffer)){
                        if(!IsOccupied(this, Vector2D(MyPos.x, MyPos.y-20), cbuffer)
                            &&
                           !IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y-10), cbuffer))
                        {
                            //OccupyHole(Vector2D(MyPos.x-10, MyPos.y-10));
                            return true;
                        }
                        if(!IsOccupied(this, Vector2D(MyPos.x, MyPos.y+20), cbuffer)
                            &&
                           !IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y+10), cbuffer))
                        {
                            //OccupyHole(Vector2D(MyPos.x-10, MyPos.y+10));
                            return true;
                        }

                    }
                    else if(MyPos.y<0){
                        if(!IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y+10), cbuffer)){
                            OccupyHole(Vector2D(MyPos.x-10, MyPos.y+10));
                            return true;
                        }
                    }
                    else if(MyPos.y >0){
                        if(!IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y-10), cbuffer)){
                            OccupyHole(Vector2D(MyPos.x-10, MyPos.y-10));
                            return true;
                        }
                    }
                }
            }
        }
        //Ball above midline
        else if(BallAboveMidLine){
            //Level 0
            if(AreSameNos(BallPos.x, MyPos.x, obuffer)){
                if(AreSamePoints(MyPos, BHupvert, obuffer)){
                    if(!IsOccupied(this, BHfrontup, cbuffer)){
                        OccupyHole(BHfrontup);
                        return true;
                    }
                }
                else if(AreSamePoints(MyPos, BHdownvert, obuffer)){
                    if(!IsOccupied(this, BHfrontdown, cbuffer)){
                        OccupyHole(BHfrontdown);
                        return true;
                    }
                }
                return true;
            }

            //Level 1
            else if(AreSameNos(BallPos.x - 10, MyPos.x, obuffer)){
                if(AreSamePoints(MyPos, BHbackup, obuffer)){
                    if(!IsOccupied(this, BHupvert, cbuffer)){
                        OccupyHole(BHupvert);
                        return true;
                    }
                }
                else if(AreSamePoints(MyPos, BHbackdown, obuffer)){
                    if(!IsOccupied(this, BHdownvert, cbuffer)){
                        //OccupyHole(BHdownvert);
                        return true;
                    }
                }
                else{
                    if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y-10), cbuffer)){
                        OccupyHole(Vector2D(MyPos.x+10, MyPos.y-10));
                        return true;
                    }
                }
            }
            //All other levels behind the ball holder
            else if(MyPos.x < BallPos.x - 15)
            {
                if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y-10), cbuffer)){
                    OccupyHole(Vector2D(MyPos.x+10, MyPos.y-10));
                    return true;
                }
            }
            //Level -1
            else if(AreSameNos(BallPos.x+10, MyPos.x, obuffer))
                return true;
            //Level -2 onwards
            else if(MyPos.x > BallPos.x + 15){
                if(AreSamePoints(Vector2D(MyPos.x-10, MyPos.y-10), BHfrontup, obuffer))
                    return true;
                if(AreSamePoints(Vector2D(MyPos.x-10, MyPos.y-10), BHfrontdown, obuffer))
                    return true;
                //Level -2 does not move, since conflict with Level 0 players at Level -1
                //if(AreSameNos(MyPos.x - 20, BallPos.x, obuffer))
                  //  return true;
                if(!IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y-10), cbuffer)){
                    OccupyHole(Vector2D(MyPos.x-10, MyPos.y-10));
                    return true;
                }
            }
        }
        //Ball below midline
        else{
            //Level 0
            if(AreSameNos(BallPos.x, MyPos.x, obuffer)){
                if(AreSamePoints(MyPos, BHupvert, obuffer)){
                    if(!IsOccupied(this, BHfrontup, cbuffer)){
                        OccupyHole(BHfrontup);
                        return true;
                    }
                }
                else if(AreSamePoints(MyPos, BHdownvert, obuffer)){
                    if(!IsOccupied(this, BHfrontdown, cbuffer)){
                        OccupyHole(BHfrontdown);
                        return true;
                    }
                }
                return true;
            }
            //Level 1
            else if(AreSameNos(BallPos.x - 10, MyPos.x, obuffer)){
                if(AreSamePoints(MyPos, BHbackup, obuffer)){
                    if(!IsOccupied(this, BHupvert, cbuffer)){
                        //OccupyHole(BHupvert);
                        return true;
                    }
                }
                else if(AreSamePoints(MyPos, BHbackdown, obuffer)){
                    if(!IsOccupied(this, BHdownvert, cbuffer)){
                        OccupyHole(BHdownvert);
                        return true;
                    }
                }
                else{
                    if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y+10), cbuffer)){
                        OccupyHole(Vector2D(MyPos.x+10, MyPos.y+10));
                        return true;
                    }
                }
            }
            //All other levels behind the ball holder
            else if(MyPos.x < BallPos.x - 15)
            {
                if(!IsOccupied(this, Vector2D(MyPos.x+10, MyPos.y+10), cbuffer)){
                    OccupyHole(Vector2D(MyPos.x+10, MyPos.y+10));
                    return true;
                }
            }
            //Level -1
            else if(AreSameNos(BallPos.x+10, MyPos.x, obuffer))
                return true;
            //Level -2 onwards
            else if(MyPos.x > BallPos.x + 15){
                if(AreSamePoints(Vector2D(MyPos.x-10, MyPos.y+10), BHfrontup, obuffer))
                    return true;
                if(AreSamePoints(Vector2D(MyPos.x-10, MyPos.y+10), BHfrontdown, obuffer))
                    return true;
                //Level -2 does not move, since conflict with Level 0 players at Level -1
                //if(AreSameNos(MyPos.x - 20, BallPos.x, obuffer))
                  //  return true;
                if(!IsOccupied(this, Vector2D(MyPos.x-10, MyPos.y+10), cbuffer)){
                    OccupyHole(Vector2D(MyPos.x-10, MyPos.y+10));
                    return true;
                }
            }
        }
    }

    Vector2D target_point = wm.self().pos();

    bool avoid_opponent = false;
    if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.9 )
    {
        const PlayerObject * nearest_opp = wm.getOpponentNearestToSelf( 5 );

        if ( nearest_opp
             && nearest_opp->pos().dist( target_point ) < 3.0 )
        {
            Vector2D add_vec = ( wm.ball().pos() - target_point );
            add_vec.setLength( 3.0 );

            long time_val = wm.time().cycle() % 60;
            if ( time_val < 20 )
            {

            }
            else if ( time_val < 40 )
            {
                target_point += add_vec.rotatedVector( 90.0 );
            }
            else
            {
                target_point += add_vec.rotatedVector( -90.0 );
            }

            target_point.x = min_max( - ServerParam::i().pitchHalfLength(),
                                      target_point.x,
                                      + ServerParam::i().pitchHalfLength() );
            target_point.y = min_max( - ServerParam::i().pitchHalfWidth(),
                                      target_point.y,
                                      + ServerParam::i().pitchHalfWidth() );
            avoid_opponent = true;
        }
    }

    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    agent->debugClient().addMessage( "KickInMove" );
    agent->debugClient().setTarget( target_point );

    double kicker_ball_dist = ( ! wm.teammatesFromBall().empty()
                                ? wm.teammatesFromBall().front()->distFromBall()
                                : 1000.0 );


    if ( ! Body_GoToPoint( target_point,
                           dist_thr,
                           dash_power
                           ).execute( agent ) )
    {
        // already there
        if ( kicker_ball_dist > 1.0 )
        {
            agent->doTurn( 120.0 );
        }
        else
        {
            Body_TurnToBall().execute( agent );
        }
    }

    Vector2D my_inertia = wm.self().inertiaFinalPoint();
    double wait_dist_buf = ( avoid_opponent
                             ? 10.0
                             : wm.ball().pos().dist( target_point ) * 0.2 + 6.0 );

    if ( my_inertia.dist( target_point ) > wait_dist_buf
         || wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.7 )
    {
        if ( ! wm.self().staminaModel().capacityIsEmpty() )
        {
            agent->debugClient().addMessage( "Sayw" );
            agent->addSayMessage( new WaitRequestMessage() );
        }
    }

    if ( kicker_ball_dist > 3.0 )
    {
        agent->setViewAction( new View_Wide() );
        agent->setNeckAction( new Neck_ScanField() );
    }
    else if ( wm.ball().distFromSelf() > 10.0
              || kicker_ball_dist > 1.0 )
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    return false;
}

/*bool SamplePlayer::PlacePlayers(PlayerAgent * agent){

    double obuffer = 2.5;
    double cbuffer = 0.5;
    std::vector<Vector2D> placement = calculatePlacementThreat();

    for(int i=0;i<placement.size();i++){
        if(!IsOccupied(this, Vector2D(placement[i].x, placement[i].y), cbuffer)){
                    OccupyHole(Vector2D(placement[i].y, placement[i].y));
                    return true;
                }
    } 
    return false;
}*/

void 
SamplePlayer::OccupyHole(Vector2D target){
/*
std::vector<Vector2D> placement = calculatePlacementThreat();

double min =10000.0;
if(placement.size()>0){
for(int i=0;i<placement.size();i++){
 if(DistanceBetweenPoints(target.x,target.y,placement[i].x,placement[i].y)< min){
        
    if(abs(target.x)<55 && abs(target.y)<35){
        mpIntransit = true;
        mpTarget = target;
        IsOccupying = true;
        std::cout<<"The target is"<<mpTarget.x<<mpTarget.y<<std::endl;
       min = DistanceBetweenPoints(target.x,target.y,placement[i].x,placement[i].y); 
     }
    }
}
}

else{
*/
if(abs(target.x)<55 && abs(target.y)<35){
        mpIntransit = true;
        //mpTarget = calculate_placement_threat(this, target);
        mpTarget = target;
        IsOccupying = true;        
    }

//}
}
double 
SamplePlayer::abs(double d){
    if (d>0.00)
        return d;
    else
        return d*(-1.00);
}

Vector2D 
SamplePlayer::RoundToNearestTens(Vector2D P){
    // This method rounds a given position to its nearest tens - for example, the rounded position for (12, -17) would be (10, -20)
    // This helps in locating nearby holes more easily
    double multX = 10.00;
    double multY = 10.00;
    if(P.x<0.00)
        multX = -10.00;
    if(P.y<0.00)
        multY = -10.00;
    int roundX = static_cast<int>((abs(P.x)+5.00)/10);
    int roundY = static_cast<int>((abs(P.y)+5.00)/10);
    Vector2D roundedTens = Vector2D(multX*roundX, multY*roundY);
    return roundedTens;
}

bool 
SamplePlayer::isRTaHole(Vector2D P){
    // This method is only for rounded tens
    // Returns true iff rounded-ten point is a hole    
    int normalX = static_cast<int>(abs(P.x)/10);
    int normalY = static_cast<int>(abs(P.y)/10);
    if(normalX%2==normalY%2)
        return true;
    else
        return false;
}

Vector2D 
SamplePlayer::RoundToNearestHole(Vector2D P){
    Vector2D roundedTens = RoundToNearestTens(P);
    if(isRTaHole(roundedTens)){
        return roundedTens;
    }
    else{
        Vector2D roundedHole;
        double diffX = P.x-roundedTens.x;
        double diffY = P.y-roundedTens.y;
            
        if(abs(diffX)<abs(diffY)){
            //Point closer to vertical axis of the diamond
            if(diffY>0)
                roundedHole = Vector2D(roundedTens.x, roundedTens.y+10);
            else
                roundedHole = Vector2D(roundedTens.x, roundedTens.y-10);
        }
        else{
            //Point closer to horizontal axis of the diamond
            if(diffX>0)
                roundedHole = Vector2D(roundedTens.x+10, roundedTens.y);
            else
                roundedHole = Vector2D(roundedTens.x-10, roundedTens.y);
        }
            return roundedHole;
    }
}

int
SamplePlayer::GetBHUnum(PlayerAgent * agent){
    const WorldModel & wm = agent->world();
    for(int i=1; i<=11; i++){
      if(isKickable(agent, i)){        
        return i;
        }
    }
    return -1;
}

bool
SamplePlayer::isKickable(PlayerAgent * agent, int unum){
    const WorldModel & wm = agent->world();
    if(wm.ourPlayer(unum)!=NULL){
        if(wm.ourPlayer(unum)->distFromBall() < ServerParam::i().defaultKickableArea()){
            return true;
        }
        return false;
    }
    else{
        return false;
    }
}

bool 
SamplePlayer::AreSamePoints(Vector2D A, Vector2D B, double buffer){
    //Check if and b are the same points +/- buffer
    if(A.dist(B)<buffer)
        return true;
    return false;
}

bool
SamplePlayer::AreSameNos(double A, double B, double buffer){
    if( abs(A-B) < buffer)
        return true;
    return false;
}

bool
SamplePlayer::IsOccupied(PlayerAgent * agent, Vector2D target, double buffer){
    //Body_TurnToPoint( target ).execute( agent );
    if(abs(target.x)>55 || abs(target.y)>35)
        return true;

    Neck_TurnToPoint( target ).execute(agent );
    const WorldModel & wm = agent->world();

    if ( target.x > wm.offsideLineX() + 1.0 ){
        // offside players are rejected.
        return true;
    }
    for(int i=2; i<=11; i++){
        if(wm.ourPlayer(i)!=NULL){
            Vector2D player_pos = wm.ourPlayer(i)->pos();
            if( AreSamePoints(player_pos, target, buffer) && i!=wm.self().unum() )
                return true;
        }
    }
    return false;
}

bool
SamplePlayer::IsOccupiedWhileDashing(PlayerAgent * agent, Vector2D target, double buffer){
    //Body_TurnToPoint( target ).execute( agent );
    if(abs(target.x)>55 || abs(target.y)>35)
        return true;
    Neck_TurnToPoint( target ).execute(agent );
    const WorldModel & wm = agent->world();
    for(int i=2; i<=11; i++){
        if(wm.ourPlayer(i)!=NULL){
            Vector2D player_pos = wm.ourPlayer(i)->pos();
            if( AreSamePoints(player_pos, target, buffer) && i!=wm.self().unum() )
                return true;
        }
    }
    return false;
}

int
SamplePlayer::IsOccupiedForPassing(PlayerAgent * agent, Vector2D target, double buffer){
    //Body_TurnToPoint( target ).execute( agent );
    if(abs(target.x)>55 || abs(target.y)>35)
        return 0;

    Neck_TurnToPoint( target ).execute(agent );
    const WorldModel & wm = agent->world();
    
    if ( target.x > wm.offsideLineX() + 1.0 ){
        // offside players are rejected.
        return false;
    }
    for(int i=2; i<=11; i++){
        if(wm.ourPlayer(i)!=NULL){
            Vector2D player_pos = wm.ourPlayer(i)->pos();
            if( AreSamePoints(player_pos, target, buffer) && i!=wm.self().unum() )
                return i;
        }
    }
    return 0;
}

int
SamplePlayer::GetOccupierUnum(PlayerAgent * agent, Vector2D target, double buffer){
    //Body_TurnToPoint( target ).execute( agent );
    Neck_TurnToPoint( target ).execute( agent );
    const WorldModel & wm = agent->world();
    for(int i=2; i<=11; i++){
        if(wm.ourPlayer(i)!=NULL){
            Vector2D player_pos = wm.ourPlayer(i)->pos();
            if( AreSamePoints(player_pos, target, buffer) && i!=wm.self().unum() )
                return i;
        }
    }
    return -1;
}

double DistanceBetweenPoints(double x1,double y1,double x2,double y2){
 
        double distance = sqrt(pow((x1-x2),2)+pow((y1-y2),2));

        return distance;
    }



    double AngleBetweenPoints(double x1,double y1,double x2,double y2,double x3,double y3){

        double angle = acos((pow(DistanceBetweenPoints(x1,y1,x2,y2),2)+pow(DistanceBetweenPoints(x1,y1,x3,y3),2)-pow(DistanceBetweenPoints(x2,y2,x3,y3),2))/(2*DistanceBetweenPoints(x1,y1,x2,y2)*DistanceBetweenPoints(x1,y1,x3,y3)));
        
        return angle;
    }

    static bool compare_first(const std::pair<double,double>& i, const std::pair<double,double>& j)
    {
        return i.first > j.first;
    }


    static bool compare_second(const std::pair<double,double>& i, const std::pair<double,double>& j)
    {
        return i.second > j.second;
    }

    double slope(double x1,double y1,double x2,double y2){

        double slope_of_line = (y2-y1)/(x2-x1);

        return slope_of_line;
    }

    double constant(double x1,double y1,double slope_of_line){

        return (y1-slope_of_line*x1);
    }

    double bisector_line_const(double c1,double c2,double a1,double a2){


        return ((c2-c1)*sqrt((pow(a2,2)+1)*(pow(a1,2)+1)));
    }

    double bisector_line_x_const(double a1,double a2){

        return ((a2*sqrt(pow(a1,2)+1))-(a1*sqrt(pow(a2,2)+1)));
    }

    double bisector_line_y_const(double a1,double a2){


        return (sqrt(pow(a2,2)+1)-sqrt(pow(a1,2)+1));
    }

    double intersecting_point_x(double c1, double c2, double a, double b){

        double x = ((c2*b - c1*a)/((a*a)+(b*b)));
        
        return x;
    }



    double intersecting_point_y(double c1, double c2, double a, double b){

        double y = ((c2*a + c1*b)/((a*a)+(b*b)));
        
        return y;
    } 

    double angle_between_two_lines(const Line2D & line1,const Line2D & line2 ){

        double theta_1 = atan(line1.getB()/line1.getA()); 
    
        double theta_2 = atan(line2.getB()/line2.getA());

        return (theta_2-theta_1);
    }


    static
    Line2D angle_bisectorof( const Vector2D & origin,
                           const AngleDeg & left,
                           const AngleDeg & right )
      {
          return Line2D( origin, AngleDeg::bisect( left, right ) );
      }




Vector2D adjust_hole(PlayerAgent * agent,double opp_dist,double min,Vector2D p1,Vector2D p2,Vector2D p3){

        const WorldModel & wm = agent->world();
        double n = min;
        double d = p2.dist(p3);

        std::cout<<"The distance d is"<<d<<std::endl;

        //modify according to the ball velocity
        Vector2D onestep_vel = Body_KickOneStep::get_max_possible_vel((p3-wm.ball().pos()).th(),wm.self().kickRate(),
          wm.ball().vel());




        //velocity of the ball is always zero.
        //Not able to get the effective velocity.

        std::cout<<"the ball position"<<wm.self().unum()<<std::endl;
        std::cout<<"the ball velocity"<<wm.ball().vel()<<std::endl;
        std::cout<<"the ball kickrate"<<wm.self().kickRate()<<std::endl;
        std::cout<<"the velocity of the player is"<<onestep_vel.x<<","<<onestep_vel.y<<std::endl;

        double ball_speed = 50.0;

        std::cout<<"the ball speed is"<<ball_speed<<std::endl;

        double time_to_reach_ball = min/ball_speed;

        std::cout<<"the time to reach the ball is"<<time_to_reach_ball<<std::endl;        

        double time_to_reach_opp = (d/(ServerParam::i().maxDashPower()));

        std::cout<<"time to reach the opponent is "<<time_to_reach_opp<<std::endl;
        //if(time_to_reach_opp > time_to_reach_ball){

        double r = n/(((time_to_reach_opp-time_to_reach_ball)*ball_speed)+1.0);

        std::cout<<"the r"<<r<<std::endl;

        double xini = r*p2.x+(1-r)*p1.x;
        double yini = r*p2.y+(1-r)*p1.y;
        
        std::cout<<"the new adhole input"<<xini<<","<<yini<<std::endl;
        
        Vector2D ad_hole = Vector2D(xini,yini);

        std::cout<<"adjusted hole is"<<ad_hole.x<<","<<ad_hole.y<<std::endl;
        return ad_hole;
    }



Vector2D SamplePlayer::calculate_placement_threat(PlayerAgent * agent, Vector2D target){

const WorldModel & wm = agent->world();
std::vector <std::pair<double, double> > opp_position;

Vector2D hole_to_filled =  Vector2D(target.x,target.y);

std::cout<<"the hole to be filled"<<target.x<<","<<target.y<<std::endl;

Vector2D kick_player_pos =  wm.self().pos();

for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(), end = wm.opponentsFromSelf().end(); it!=end; ++it){

    if(DistanceBetweenPoints(((*it)->pos().x),((*it)->pos().y),kick_player_pos.x,kick_player_pos.y) <=20.0){
                    opp_position.push_back(std::make_pair((*it)->pos().x,(*it)->pos().y));

                    std::cout<<"the position of the " <<(*it)->pos().x<<","<<(*it)->pos().y<<std::endl;
                }
}

//return hole_to_filled;

if(!opp_position.empty() && (target.x > kick_player_pos.x)) {

 std::cout<<"enter in to the left side"<<std::endl;

    std::sort(opp_position.begin(),opp_position.end(),compare_second);

 std::cout<<"After Sort"<<std::endl;

    std::vector <std::pair<double,Vector2D> > regenrated_holes;

    for(int i=0;i<opp_position.size()-1;i++){

         std::cout<<"In the first for loop"<<std::endl;

        double min = 10000.0;
        double opp_dist=0.0;
        Vector2D hole_point = Vector2D(0.0,0.0);
        Vector2D best_adust_hole = Vector2D(0.0,0.0);

        for(int j=i+1;j<opp_position.size();j++){

             std::cout<<"enter in to the left side"<<std::endl;

            if(kick_player_pos.x < opp_position[i].first && kick_player_pos.x < opp_position[j].first){

                Vector2D opp_pos_1 = Vector2D(opp_position[i].first,opp_position[i].second);

                 std::cout<<"The poition of the opp1"<<opp_pos_1.x<<","<<opp_pos_1.y<<std::endl;

                Vector2D opp_pos_2 = Vector2D(opp_position[j].first,opp_position[j].second);
                  
                  std::cout<<"The poition of the opp2"<<opp_pos_2.x<<","<<opp_pos_2.y<<std::endl;  

                Line2D kp_to_opp1(kick_player_pos,opp_pos_1);
                Line2D kp_to_opp2(kick_player_pos,opp_pos_2);

                std::cout<<"the y component of line"<<kp_to_opp1.getB()<<std::endl;
                std::cout<<"the x component of line"<<kp_to_opp1.getA()<<std::endl;

                double theta_1 = atan(kp_to_opp1.getB()/kp_to_opp1.getA()); 

                std::cout<<"the left angle for bisec is"<<theta_1<<std::endl;

                AngleDeg left_angle(theta_1);

                double theta_2 = atan(kp_to_opp2.getB()/kp_to_opp2.getA());

                std::cout<<"the right angle for bisec is"<<theta_2<<std::endl;

                AngleDeg right_angle(theta_2);

                Line2D angle_bisector_kp = angle_bisectorof(kick_player_pos,left_angle,right_angle);

                for(int l=0;l<opp_position.size();l++){

                    Vector2D opp_th =Vector2D(opp_position[l].first,opp_position[l].second);
                    
                    Line2D line1_kp(kick_player_pos,opp_th);

                    //if(abs(angle_between_two_lines(line1_kp,angle_bisector_kp))<=90.0){

                        Vector2D opp_3 = Vector2D(opp_position[l].first,opp_position[l].second);

                        Line2D perpendicular_line = angle_bisector_kp.perpendicular(opp_3);


                        Vector2D intersection_point = angle_bisector_kp.intersection(perpendicular_line);

                        if(kick_player_pos.dist(intersection_point) < min && kick_player_pos.dist(intersection_point)>opp_3.dist(intersection_point)) {

                            min=kick_player_pos.dist(intersection_point);
                            hole_point = intersection_point;
                            opp_dist = opp_3.dist(intersection_point);

                            if(kick_player_pos.dist(adjust_hole(agent,opp_dist,min,kick_player_pos,intersection_point,opp_3)) < kick_player_pos.dist(best_adust_hole)) {
                                best_adust_hole = adjust_hole(agent,opp_dist,min,kick_player_pos,intersection_point,opp_3);
                            }
                        }

                    //}
                }

            }

          i=j;
          j=opp_position.size(); 
        }


       regenrated_holes.push_back(std::make_pair(kick_player_pos.dist(best_adust_hole),best_adust_hole));

}


int r1=0;
int r2 =0;
Vector2D hole_x = Vector2D(0.0,0.0);

for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(),end= wm.opponentsFromSelf().end();it!=end;++it){

    if((*it)->pos().x > wm.self().pos().x && (*it)->pos().y<wm.self().pos().y ){

        if(r1>1 && r2 >1)
            break;
        else{
            r1++;
            
        }
            
    }

    else if((*it)->pos().x > wm.self().pos().x && (*it)->pos().y>wm.self().pos().y ){

                    if(r1>1 && r2 >1)
                        break;
                    else
                    {
                        r2++;
                    hole_x = Vector2D((*it)->pos().x,(*it)->pos().y);
                    }

    }

    else{

        continue;
    }

}

if(r1+r2 ==1){
 
 Vector2D best_adust_hole = adjust_hole(agent,kick_player_pos.dist(hole_x),kick_player_pos.dist(target),kick_player_pos,hole_x,target);

 regenrated_holes.push_back(std::make_pair(kick_player_pos.dist(best_adust_hole),best_adust_hole));

}

int count=0;
int min_count=10000;
int beta_region_middle = 0;
int beta_region_left = 0;
int beta_region_right = 0;
double old_beta_threat = 10000.0;


if(!regenrated_holes.empty()){

        for(int r=0;r<regenrated_holes.size();r++){

              for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(), end = wm.opponentsFromSelf().end();it!=end; ++it){

                if(DistanceBetweenPoints(((*it)->pos().x),((*it)->pos().y),regenrated_holes[r].second.x,regenrated_holes[r].second.y) <=5.0){
                    if((*it)->pos().x<=regenrated_holes[r].second.x+1.5 && (*it)->pos().x>=regenrated_holes[r].second.x-1.5) {
                     beta_region_middle++;        
                    }
                    else if((*it)->pos().x<=regenrated_holes[r].second.x-1.5){
                        beta_region_left++;
                    }
                    else{
                        beta_region_right++;
                    }
                   
                }
            }
            

            int N = beta_region_right+beta_region_left+beta_region_middle;

            double wbl = beta_region_left * 0.25;
            double wbm = beta_region_middle * 0.50;
            double wbr = beta_region_right * 0.25;

            double beta = (wbl+wbm+wbr)/N;

            //when a player should go to hole or not
           
           if(old_beta_threat > beta*regenrated_holes[r].first)
           {
                    hole_to_filled = Vector2D(regenrated_holes[r].second.x,regenrated_holes[r].second.y);            
                    old_beta_threat = beta*regenrated_holes[r].first;
           }

           /* if(count<min_count){

                hole_to_filled = Vector2D(regenrated_holes[r].second.x,regenrated_holes[r].second.y);
            }*/  
        } 

    }

return hole_to_filled;
}

else if(!opp_position.empty() && (target.x < kick_player_pos.x)){

        std::sort(opp_position.begin(),opp_position.end(),compare_second);

 std::cout<<"After Sort"<<std::endl;

    std::vector <std::pair<double,Vector2D> > regenrated_holes;

    for(int i=0;i<opp_position.size()-1;i++){

         std::cout<<"In the first for loop"<<std::endl;

        double min = 10000.0;
        double opp_dist=0.0;
        Vector2D hole_point = Vector2D(0.0,0.0);
        Vector2D best_adust_hole = Vector2D(0.0,0.0);

        for(int j=i+1;j<opp_position.size();j++){

             std::cout<<"enter in to the left side"<<std::endl;

            if(kick_player_pos.x < opp_position[i].first && kick_player_pos.x < opp_position[j].first){

                Vector2D opp_pos_1 = Vector2D(opp_position[i].first,opp_position[i].second);

                 std::cout<<"The poition of the opp1"<<opp_pos_1.x<<","<<opp_pos_1.y<<std::endl;

                Vector2D opp_pos_2 = Vector2D(opp_position[j].first,opp_position[j].second);
                  
                  std::cout<<"The poition of the opp2"<<opp_pos_2.x<<","<<opp_pos_2.y<<std::endl;  

                Line2D kp_to_opp1(kick_player_pos,opp_pos_1);
                Line2D kp_to_opp2(kick_player_pos,opp_pos_2);

                std::cout<<"the y component of line"<<kp_to_opp1.getB()<<std::endl;
                std::cout<<"the x component of line"<<kp_to_opp1.getA()<<std::endl;

                double theta_1 = atan(kp_to_opp1.getB()/kp_to_opp1.getA()); 

                std::cout<<"the left angle for bisec is"<<theta_1<<std::endl;

                AngleDeg left_angle(theta_1);

                double theta_2 = atan(kp_to_opp2.getB()/kp_to_opp2.getA());

                std::cout<<"the right angle for bisec is"<<theta_2<<std::endl;

                AngleDeg right_angle(theta_2);

                Line2D angle_bisector_kp = angle_bisectorof(kick_player_pos,left_angle,right_angle);

                for(int l=0;l<opp_position.size();l++){

                    Vector2D opp_th =Vector2D(opp_position[l].first,opp_position[l].second);
                    
                    Line2D line1_kp(kick_player_pos,opp_th);

                    //if(abs(angle_between_two_lines(line1_kp,angle_bisector_kp))<=90.0){

                        Vector2D opp_3 = Vector2D(opp_position[l].first,opp_position[l].second);

                        Line2D perpendicular_line = angle_bisector_kp.perpendicular(opp_3);


                        Vector2D intersection_point = angle_bisector_kp.intersection(perpendicular_line);

                        if(kick_player_pos.dist(intersection_point) < min && kick_player_pos.dist(intersection_point)>opp_3.dist(intersection_point)) {

                            min=kick_player_pos.dist(intersection_point);
                            hole_point = intersection_point;
                            opp_dist = opp_3.dist(intersection_point);

                            if(kick_player_pos.dist(adjust_hole(agent,opp_dist,min,kick_player_pos,intersection_point,opp_3)) < kick_player_pos.dist(best_adust_hole)) {
                                best_adust_hole = adjust_hole(agent,opp_dist,min,kick_player_pos,intersection_point,opp_3);
                            }
                        }

                    //}
                }

            }

          i=j;
          j=opp_position.size(); 
        }


       regenrated_holes.push_back(std::make_pair(kick_player_pos.dist(best_adust_hole),best_adust_hole));

}


int r1=0;
int r2 =0;
Vector2D hole_x = Vector2D(0.0,0.0);

for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(),end= wm.opponentsFromSelf().end();it!=end;++it){

    if((*it)->pos().x < wm.self().pos().x && (*it)->pos().y>wm.self().pos().y ){

        if(r1>1 && r2 >1)
            break;
        else{
            r1++;
            
        }
            
    }

    else if((*it)->pos().x < wm.self().pos().x && (*it)->pos().y<wm.self().pos().y ){

                    if(r1>1 && r2 >1)
                        break;
                    else
                    {
                        r2++;
                    hole_x = Vector2D((*it)->pos().x,(*it)->pos().y);
                    }

    }

    else{

        continue;
    }

}

if(r1+r2 ==1){
 
 Vector2D best_adust_hole = adjust_hole(agent,kick_player_pos.dist(hole_x),kick_player_pos.dist(target),kick_player_pos,hole_x,target);

 regenrated_holes.push_back(std::make_pair(kick_player_pos.dist(best_adust_hole),best_adust_hole));

}

int count=0;
int min_count=10000;
int beta_region_middle = 0;
int beta_region_left = 0;
int beta_region_right = 0;
double old_beta_threat = 10000.0;


if(!regenrated_holes.empty()){

        for(int r=0;r<regenrated_holes.size();r++){

              for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(), end = wm.opponentsFromSelf().end();it!=end; ++it){

                if(DistanceBetweenPoints(((*it)->pos().x),((*it)->pos().y),regenrated_holes[r].second.x,regenrated_holes[r].second.y) <=5.0){
                    if((*it)->pos().x<=regenrated_holes[r].second.x+1.5 && (*it)->pos().x>=regenrated_holes[r].second.x-1.5) {
                     beta_region_middle++;        
                    }
                    else if((*it)->pos().x<=regenrated_holes[r].second.x-1.5){
                        beta_region_left++;
                    }
                    else{
                        beta_region_right++;
                    }
                   
                }
            }
            

            int N = beta_region_right+beta_region_left+beta_region_middle;

            double wbl = beta_region_left * 0.25;
            double wbm = beta_region_middle * 0.50;
            double wbr = beta_region_right * 0.25;

            double beta = (wbl+wbm+wbr)/N;

            //when a player should go to hole or not
           
           if(old_beta_threat > beta*regenrated_holes[r].first)
           {
                    hole_to_filled = Vector2D(regenrated_holes[r].second.x,regenrated_holes[r].second.y);            
                    old_beta_threat = beta*regenrated_holes[r].first;
           }

           /* if(count<min_count){

                hole_to_filled = Vector2D(regenrated_holes[r].second.x,regenrated_holes[r].second.y);
            }*/  
        } 

    }

return hole_to_filled;
}

else if(!opp_position.empty() && (target.y > kick_player_pos.y)){

    std::sort(opp_position.begin(),opp_position.end(),compare_first);

    std::cout<<"After Sort"<<std::endl;

    std::vector <std::pair<double,Vector2D> > regenrated_holes;

    for(int i=0;i<opp_position.size()-1;i++){

         std::cout<<"In the first for loop"<<std::endl;

        double min = 10000.0;
        double opp_dist=0.0;
        Vector2D hole_point = Vector2D(0.0,0.0);
        Vector2D best_adust_hole = Vector2D(0.0,0.0);

        for(int j=i+1;j<opp_position.size();j++){

             std::cout<<"enter in to the left side"<<std::endl;

            if(kick_player_pos.x < opp_position[i].first && kick_player_pos.x < opp_position[j].first){

                Vector2D opp_pos_1 = Vector2D(opp_position[i].first,opp_position[i].second);

                 std::cout<<"The poition of the opp1"<<opp_pos_1.x<<","<<opp_pos_1.y<<std::endl;

                Vector2D opp_pos_2 = Vector2D(opp_position[j].first,opp_position[j].second);
                  
                  std::cout<<"The poition of the opp2"<<opp_pos_2.x<<","<<opp_pos_2.y<<std::endl;  

                Line2D kp_to_opp1(kick_player_pos,opp_pos_1);
                Line2D kp_to_opp2(kick_player_pos,opp_pos_2);

                std::cout<<"the y component of line"<<kp_to_opp1.getB()<<std::endl;
                std::cout<<"the x component of line"<<kp_to_opp1.getA()<<std::endl;

                double theta_1 = atan(kp_to_opp1.getB()/kp_to_opp1.getA()); 

                std::cout<<"the left angle for bisec is"<<theta_1<<std::endl;

                AngleDeg left_angle(theta_1);

                double theta_2 = atan(kp_to_opp2.getB()/kp_to_opp2.getA());

                std::cout<<"the right angle for bisec is"<<theta_2<<std::endl;

                AngleDeg right_angle(theta_2);

                Line2D angle_bisector_kp = angle_bisectorof(kick_player_pos,left_angle,right_angle);

                for(int l=0;l<opp_position.size();l++){

                    Vector2D opp_th =Vector2D(opp_position[l].first,opp_position[l].second);
                    
                    Line2D line1_kp(kick_player_pos,opp_th);

                    //if(abs(angle_between_two_lines(line1_kp,angle_bisector_kp))<=90.0){

                        Vector2D opp_3 = Vector2D(opp_position[l].first,opp_position[l].second);

                        Line2D perpendicular_line = angle_bisector_kp.perpendicular(opp_3);


                        Vector2D intersection_point = angle_bisector_kp.intersection(perpendicular_line);

                        if(kick_player_pos.dist(intersection_point) < min && kick_player_pos.dist(intersection_point)>opp_3.dist(intersection_point)) {

                            min=kick_player_pos.dist(intersection_point);
                            hole_point = intersection_point;
                            opp_dist = opp_3.dist(intersection_point);

                            if(kick_player_pos.dist(adjust_hole(agent,opp_dist,min,kick_player_pos,intersection_point,opp_3)) < kick_player_pos.dist(best_adust_hole)) {
                                best_adust_hole = adjust_hole(agent,opp_dist,min,kick_player_pos,intersection_point,opp_3);
                            }
                        }

                    //}
                }

            }

          i=j;
          j=opp_position.size(); 
        }


       regenrated_holes.push_back(std::make_pair(kick_player_pos.dist(best_adust_hole),best_adust_hole));

}


int r1=0;
int r2 =0;
Vector2D hole_x = Vector2D(0.0,0.0);

for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(),end= wm.opponentsFromSelf().end();it!=end;++it){

    if((*it)->pos().x > wm.self().pos().x && (*it)->pos().y>wm.self().pos().y ){

        if(r1>1 && r2 >1)
            break;
        else{
            r1++;
            
        }
            
    }

    else if((*it)->pos().x < wm.self().pos().x && (*it)->pos().y>wm.self().pos().y ){

                    if(r1>1 && r2 >1)
                        break;
                    else
                    {
                        r2++;
                    hole_x = Vector2D((*it)->pos().x,(*it)->pos().y);
                    }

    }

    else{

        continue;
    }

}

if(r1+r2 ==1){
 
 Vector2D best_adust_hole = adjust_hole(agent,kick_player_pos.dist(hole_x),kick_player_pos.dist(target),kick_player_pos,hole_x,target);

 regenrated_holes.push_back(std::make_pair(kick_player_pos.dist(best_adust_hole),best_adust_hole));

}

int count=0;
int min_count=10000;
int beta_region_middle = 0;
int beta_region_left = 0;
int beta_region_right = 0;
double old_beta_threat = 10000.0;


if(!regenrated_holes.empty()){

        for(int r=0;r<regenrated_holes.size();r++){

              for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(), end = wm.opponentsFromSelf().end();it!=end; ++it){

                if(DistanceBetweenPoints(((*it)->pos().x),((*it)->pos().y),regenrated_holes[r].second.x,regenrated_holes[r].second.y) <=5.0){
                    if((*it)->pos().x<=regenrated_holes[r].second.x+1.5 && (*it)->pos().x>=regenrated_holes[r].second.x-1.5) {
                     beta_region_middle++;        
                    }
                    else if((*it)->pos().x<=regenrated_holes[r].second.x-1.5){
                        beta_region_left++;
                    }
                    else{
                        beta_region_right++;
                    }
                   
                }
            }
            

            int N = beta_region_right+beta_region_left+beta_region_middle;

            double wbl = beta_region_left * 0.25;
            double wbm = beta_region_middle * 0.50;
            double wbr = beta_region_right * 0.25;

            double beta = (wbl+wbm+wbr)/N;

            //when a player should go to hole or not
           
           if(old_beta_threat > beta*regenrated_holes[r].first)
           {
                    hole_to_filled = Vector2D(regenrated_holes[r].second.x,regenrated_holes[r].second.y);            
                    old_beta_threat = beta*regenrated_holes[r].first;
           }

           /* if(count<min_count){

                hole_to_filled = Vector2D(regenrated_holes[r].second.x,regenrated_holes[r].second.y);
            }*/  
        } 

    }

return hole_to_filled;
}
/*
else {

    
    std::sort(opp_position.begin(),opp_position.end(),compare_first);

    std::cout<<"After Sort"<<std::endl;

    std::vector <std::pair<double,Vector2D> > regenrated_holes;

    for(int i=0;i<opp_position.size()-1;i++){

         std::cout<<"In the first for loop"<<std::endl;

        double min = 10000.0;
        double opp_dist=0.0;
        Vector2D hole_point = Vector2D(0.0,0.0);
        Vector2D best_adust_hole = Vector2D(0.0,0.0);

        for(int j=i+1;j<opp_position.size();j++){

            std::cout<<"enter in to the left side"<<std::endl;

            if(kick_player_pos.x < opp_position[i].first && kick_player_pos.x < opp_position[j].first){

                Vector2D opp_pos_1 = Vector2D(opp_position[i].first,opp_position[i].second);

                 std::cout<<"The poition of the opp1"<<opp_pos_1.x<<","<<opp_pos_1.y<<std::endl;

                Vector2D opp_pos_2 = Vector2D(opp_position[j].first,opp_position[j].second);
                  
                  std::cout<<"The poition of the opp2"<<opp_pos_2.x<<","<<opp_pos_2.y<<std::endl;  

                Line2D kp_to_opp1(kick_player_pos,opp_pos_1);
                Line2D kp_to_opp2(kick_player_pos,opp_pos_2);

                std::cout<<"the y component of line"<<kp_to_opp1.getB()<<std::endl;
                std::cout<<"the x component of line"<<kp_to_opp1.getA()<<std::endl;

                double theta_1 = atan(kp_to_opp1.getB()/kp_to_opp1.getA()); 

                std::cout<<"the left angle for bisec is"<<theta_1<<std::endl;

                AngleDeg left_angle(theta_1);

                double theta_2 = atan(kp_to_opp2.getB()/kp_to_opp2.getA());

                std::cout<<"the right angle for bisec is"<<theta_2<<std::endl;

                AngleDeg right_angle(theta_2);

                Line2D angle_bisector_kp = angle_bisectorof(kick_player_pos,left_angle,right_angle);

                for(int l=0;l<opp_position.size();l++){

                    Vector2D opp_th =Vector2D(opp_position[l].first,opp_position[l].second);
                    
                    Line2D line1_kp(kick_player_pos,opp_th);

                    //if(abs(angle_between_two_lines(line1_kp,angle_bisector_kp))<=90.0){

                        Vector2D opp_3 = Vector2D(opp_position[l].first,opp_position[l].second);

                        Line2D perpendicular_line = angle_bisector_kp.perpendicular(opp_3);


                        Vector2D intersection_point = angle_bisector_kp.intersection(perpendicular_line);

                        if(kick_player_pos.dist(intersection_point) < min && kick_player_pos.dist(intersection_point)>opp_3.dist(intersection_point)) {

                            min=kick_player_pos.dist(intersection_point);
                            hole_point = intersection_point;
                            opp_dist = opp_3.dist(intersection_point);

                            if(kick_player_pos.dist(adjust_hole(agent,opp_dist,min,kick_player_pos,intersection_point,opp_3)) < kick_player_pos.dist(best_adust_hole)) {
                                best_adust_hole = adjust_hole(agent,opp_dist,min,kick_player_pos,intersection_point,opp_3);
                            }
                        }

                    //}
                }

            }

          i=j;
          j=opp_position.size(); 
        }


       regenrated_holes.push_back(std::make_pair(kick_player_pos.dist(best_adust_hole),best_adust_hole));

}


int r1=0;
int r2 =0;
Vector2D hole_x = Vector2D(0.0,0.0);

for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(),end= wm.opponentsFromSelf().end();it!=end;++it){

    if((*it)->pos().x > wm.self().pos().x && (*it)->pos().y<wm.self().pos().y ){

        if(r1>1 && r2 >1)
            break;
        else{
            r1++;
            
        }
            
    }

    else if((*it)->pos().x < wm.self().pos().x && (*it)->pos().y<wm.self().pos().y ){

                    if(r1>1 && r2 >1)
                        break;
                    else
                    {
                        r2++;
                    hole_x = Vector2D((*it)->pos().x,(*it)->pos().y);
                    }

    }

    else{

        continue;
    }

}

if(r1+r2 ==1){
 
 Vector2D best_adust_hole = adjust_hole(agent,kick_player_pos.dist(hole_x),kick_player_pos.dist(target),kick_player_pos,hole_x,target);

 regenrated_holes.push_back(std::make_pair(kick_player_pos.dist(best_adust_hole),best_adust_hole));

}

int count=0;
int min_count=10000;
int beta_region_middle = 0;
int beta_region_left = 0;
int beta_region_right = 0;
double old_beta_threat = 10000.0;


if(!regenrated_holes.empty()){

        for(int r=0;r<regenrated_holes.size();r++){

              for(PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(), end = wm.opponentsFromSelf().end();it!=end; ++it){

                if(DistanceBetweenPoints(((*it)->pos().x),((*it)->pos().y),regenrated_holes[r].second.x,regenrated_holes[r].second.y) <=5.0){
                    if((*it)->pos().x<=regenrated_holes[r].second.x+1.5 && (*it)->pos().x>=regenrated_holes[r].second.x-1.5) {
                     beta_region_middle++;        
                    }
                    else if((*it)->pos().x<=regenrated_holes[r].second.x-1.5){
                        beta_region_left++;
                    }
                    else{
                        beta_region_right++;
                    }
                   
                }
            }
            

            int N = beta_region_right+beta_region_left+beta_region_middle;

            double wbl = beta_region_left * 0.25;
            double wbm = beta_region_middle * 0.50;
            double wbr = beta_region_right * 0.25;

            double beta = (wbl+wbm+wbr)/N;

            //when a player should go to hole or not
           
           if(old_beta_threat > beta*regenrated_holes[r].first)
           {
                    hole_to_filled = Vector2D(regenrated_holes[r].second.x,regenrated_holes[r].second.y);            
                    old_beta_threat = beta*regenrated_holes[r].first;
           }

           // if(count<min_count){

             //   hole_to_filled = Vector2D(regenrated_holes[r].second.x,regenrated_holes[r].second.y);
            //} 
        } 

    }

return hole_to_filled;
}
*/
return hole_to_filled;
}



/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handleActionStart()
{

}







/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handleActionEnd()
{
    if ( world().self().posValid() )
    {
#if 0
    const ServerParam & SP = ServerParam::i();
        //
        // inside of pitch
        //

        // top,lower
        debugClient().addLine( Vector2D( world().ourOffenseLineX(),
                                         -SP.pitchHalfWidth() ),
                               Vector2D( world().ourOffenseLineX(),
                                         -SP.pitchHalfWidth() + 3.0 ) );
        // top,lower
        debugClient().addLine( Vector2D( world().ourDefenseLineX(),
                                         -SP.pitchHalfWidth() ),
                               Vector2D( world().ourDefenseLineX(),
                                         -SP.pitchHalfWidth() + 3.0 ) );

        // bottom,upper
        debugClient().addLine( Vector2D( world().theirOffenseLineX(),
                                         +SP.pitchHalfWidth() - 3.0 ),
                               Vector2D( world().theirOffenseLineX(),
                                         +SP.pitchHalfWidth() ) );
        //
        debugClient().addLine( Vector2D( world().offsideLineX(),
                                         world().self().pos().y - 15.0 ),
                               Vector2D( world().offsideLineX(),
                                         world(Opponenthasball
Opponenthasball
Opponenthasball
Opponenthasball
Opponenthasball
Opponenthasball).self().pos().y + 15.0 ) );

        // outside of pitch

        // top,upper
        debugClient().addLine( Vector2D( world().ourOffensePlayerLineX(),
                                         -SP.pitchHalfWidth() - 3.0 ),
                               Vector2D( world().ourOffensePlayerLineX(),
                                         -SP.pitchHalfWidth() ) );
        // top,upper
        debugClient().addLine( Vector2D( world().ourDefensePlayerLineX(),
                                         -SP.pitchHalfWidth() - 3.0 ),
                               Vector2D( world().ourDefensePlayerLineX(),
                                         -SP.pitchHalfWidth() ) );
        // bottom,lower
        debugClient().addLine( Vector2D( world(Opponenthasball
O).theirOffensePlayerLineX(),
                                         +SP.pitchHalfWidth() ),
                               Vector2D( world().theirOffensePlayerLineX(),
                                         +SP.pitchHalfWidth() + 3.0 ) );
        // bottom,lower
        debugClient().addLine( Vector2D( world().theirDefensePlayerLineX(),
                                         +SP.pitchHalfWidth() ),
                               Vector2D( world().theirDefensePlayerLineX(),
                                         +SP.pitchHalfWidth() + 3.0 ) );
#else
        // top,lower
        debugClient().addLine( Vector2D( world().ourDefenseLineX(),
                                         world().self().pos().y - 2.0 ),
                               Vector2D( world().ourDefenseLineX(),
                                         world().self().pos().y + 2.0 ) );

        //
        debugClient().addLine( Vector2D( world().offsideLineX(),
                                         world().self().pos().y - 15.0 ),
                               Vector2D( world().offsideLineX(),
                                         world().self().pos().y + 15.0 ) );
#endif
    }

    //
    // ball position & velocity
    //
    dlog.addText( Logger::WORLD,
                  "WM: BALL pos=(%lf, %lf), vel=(%lf, %lf, r=%lf, ang=%lf)",
                  world().ball().pos().x,
                  world().ball().pos().y,
                  world().ball().vel().x,
                  world().ball().vel().y,
                  world().ball().vel().r(),
                  world().ball().vel().th().degree() );


    dlog.addText( Logger::WORLD,
                  "WM: SELF move=(%lf, %lf, r=%lf, th=%lf)",
                  world().self().lastMove().x,
                  world().self().lastMove().y,
                  world().self().lastMove().r(),
                  world().self().lastMove().th().degree() );

    Vector2D diff = world().ball().rpos() - world().ball().rposPrev();
    dlog.addText( Logger::WORLD,
                  "WM: BALL rpos=(%lf %lf) prev_rpos=(%lf %lf) diff=(%lf %lf)",
                  world().ball().rpos().x,
                  world().ball().rpos().y,
                  world().ball().rposPrev().x,
                  world().ball().rposPrev().y,
                  diff.x,
                  diff.y );

    Vector2D ball_move = diff + world().self().lastMove();
    Vector2D diff_vel = ball_move * ServerParam::i().ballDecay();
    dlog.addText( Logger::WORLD,
                  "---> ball_move=(%lf %lf) vel=(%lf, %lf, r=%lf, th=%lf)",
                  ball_move.x,
                  ball_move.y,
                  diff_vel.x,
                  diff_vel.y,
                  diff_vel.r(),
                  diff_vel.th().degree() );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handleServerParam()
{
    if ( KickTable::instance().createTables() )
    {
        std::cerr << world().teamName() << ' '
                  << world().self().unum() << ": "
                  << " KickTable created."
                  << std::endl;
    }
    else
    {
        std::cerr << world().teamName() << ' '
                  << world().self().unum() << ": "
                  << " KickTable failed..."
                  << std::endl;
        M_client->setServerAlive( false );
    }


    if ( ServerParam::i().keepawayMode() )
    {
        std::cerr << "set Keepaway mode communication." << std::endl;
        M_communication = Communication::Ptr( new KeepawayCommunication() );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handlePlayerParam()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handlePlayerType()
{

}

/*-------------------------------------------------------------------*/
/*!
  communication decision.
  virtual method in super class
*/
void
SamplePlayer::communicationImpl()
{
    if ( M_communication )
    {
        M_communication->execute( this );
    }
}

/*-------------------------------------------------------------------*/
/*!
*/
bool
SamplePlayer::doPreprocess()
{
    // check tackle expires
    // check self position accuracy
    // ball search
    // check queued intention
    // check simultaneous kick

    const WorldModel & wm = this->world();

    dlog.addText( Logger::TEAM,
                  __FILE__": (doPreProcess)" );

    //
    // freezed by tackle effect
    //
        if ( wm.self().isFrozen() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": tackle wait. expires= %d",
                      wm.self().tackleExpires() );
        // face neck to ball
        this->setViewAction( new View_Tactical() );
        this->setNeckAction( new Neck_TurnToBallOrScan() );
        return true;
    }
    

    //
    // BeforeKickOff or AfterGoal. jump to the initial position
    //
    if ( wm.gameMode().type() == GameMode::BeforeKickOff
         || wm.gameMode().type() == GameMode::AfterGoal_ )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": before_kick_off" );
        Vector2D move_point =  Strategy::i().getPosition( wm.self().unum() );
        Bhv_CustomBeforeKickOff( move_point ).execute( this );
        this->setViewAction( new View_Tactical() );
        Opponenthasball = false;
        return true;
    }

    //
    // self localization error
    //
    
    if ( ! wm.self().posValid() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": invalid my pos" );
        Bhv_Emergency().execute( this ); // includes change view
        return true;
    }
    

    //
    // ball localization error
    //
    
    const int count_thr = ( wm.self().goalie()
                            ? 10
                            : 5 );
    if ( wm.ball().posCount() > count_thr
         || ( wm.gameMode().type() != GameMode::PlayOn
              && wm.ball().seenPosCount() > count_thr + 10 ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": search ball" );
        this->setViewAction( new View_Tactical() );
        Bhv_NeckBodyToBall().execute( this );
        return true;
    }
    

    //
    // set default change view
    //

    this->setViewAction( new View_Tactical() );

    //
    // check shoot chance
    //
    
    if ( doShoot() )
    {
        std::cout<<"doShoot"<<std::endl;
        std::cout<<"*******************************************************************************"<<std::endl;

        return true;
    }
    

    //
    // check queued action
    //
    
    if ( this->doIntention() )
    {   
        std::cout<<"doIntention------------------------------------------------------------"<<std::endl;
        std::cout<<"*******************************************************************************"<<std::endl;
        dlog.addText( Logger::TEAM,
                      __FILE__": do queued intention" );
        return true;
    }

    //
    // check simultaneous kick
    //
    
    if ( doForceKick() )
    {   
        std::cout<<"doForceKick--------------------------------------------------------------"<<std::endl;
        std::cout<<"*******************************************************************************"<<std::endl;
        return true;
    }
    

    //
    // check pass message
    //
    
    if ( doHeardPassReceive() )
    {
        std::cout<<"doHeardPassReceive------------------------------------------------------"<<std::endl;
        std::cout<<"*******************************************************************************"<<std::endl;
        return true;
    }
    

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::doShoot()
{
    const WorldModel & wm = this->world();

    if ( wm.gameMode().type() != GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Bhv_StrictCheckShoot().execute( this ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": shooted" );

        // reset intention
        this->setIntention( static_cast< SoccerIntention * >( 0 ) );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::doForceKick()
{
    const WorldModel & wm = this->world();

    if ( wm.gameMode().type() == GameMode::PlayOn
         && ! wm.self().goalie()
         && wm.self().isKickable()
         && wm.existKickableOpponent() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": simultaneous kick" );
        this->debugClient().addMessage( "SimultaneousKick" );
        Vector2D goal_pos( ServerParam::i().pitchHalfLength(), 0.0 );

        if ( wm.self().pos().x > 36.0
             && wm.self().pos().absY() > 10.0 )
        {
            goal_pos.x = 45.0;
            dlog.addText( Logger::TEAM,
                          __FILE__": simultaneous kick cross type" );
        }
        Body_KickOneStep( goal_pos,
                          ServerParam::i().ballSpeedMax()
                          ).execute( this );
        this->setNeckAction( new Neck_ScanField() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::doHeardPassReceive()
{
    const WorldModel & wm = this->world();

    if ( wm.audioMemory().passTime() != wm.time()
         || wm.audioMemory().pass().empty()
         || wm.audioMemory().pass().front().receiver_ != wm.self().unum() )
    {

        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    Vector2D intercept_pos = wm.ball().inertiaPoint( self_min );
    Vector2D heard_pos = wm.audioMemory().pass().front().receive_pos_;

    dlog.addText( Logger::TEAM,
                  __FILE__":  (doHeardPassReceive) heard_pos(%.2f %.2f) intercept_pos(%.2f %.2f)",
                  heard_pos.x, heard_pos.y,
                  intercept_pos.x, intercept_pos.y );

    if ( ! wm.existKickableTeammate()
         && wm.ball().posCount() <= 1
         && wm.ball().velCount() <= 1
         && self_min < 20
         //&& intercept_pos.dist( heard_pos ) < 3.0 ) //5.0 )
         )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doHeardPassReceive) intercept cycle=%d. intercept",
                      self_min );
        this->debugClient().addMessage( "Comm:Receive:Intercept" );
        Body_Intercept().execute( this );
        this->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doHeardPassReceive) intercept cycle=%d. go to receive point",
                      self_min );
        this->debugClient().setTarget( heard_pos );
        this->debugClient().addMessage( "Comm:Receive:GoTo" );
        Body_GoToPoint( heard_pos,
                    0.5,
                        ServerParam::i().maxDashPower()
                        ).execute( this );
        this->setNeckAction( new Neck_TurnToBall() );
    }

    this->setIntention( new IntentionReceive( heard_pos,
                                              ServerParam::i().maxDashPower(),
                                              0.9,
                                              5,
                                              wm.time() ) );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
FieldEvaluator::ConstPtr
SamplePlayer::getFieldEvaluator() const
{
    return M_field_evaluator;
}

/*-------------------------------------------------------------------*/
/*!

*/
FieldEvaluator::ConstPtr
SamplePlayer::createFieldEvaluator() const
{
    return FieldEvaluator::ConstPtr( new SampleFieldEvaluator );
}


/*-------------------------------------------------------------------*/
/*!
*/
#include "actgen_cross.h"
#include "actgen_direct_pass.h"
#include "actgen_self_pass.h"
#include "actgen_strict_check_pass.h"
#include "actgen_short_dribble.h"
#include "actgen_simple_dribble.h"
#include "actgen_shoot.h"
#include "actgen_action_chain_length_filter.h"

ActionGenerator::ConstPtr
SamplePlayer::createActionGenerator() const
{
    CompositeActionGenerator * g = new CompositeActionGenerator();

    //
    // shoot
    //
    g->addGenerator( new ActGen_RangeActionChainLengthFilter
                     ( new ActGen_Shoot(),
                       2, ActGen_RangeActionChainLengthFilter::MAX ) );

    //
    // strict check pass
    //
    g->addGenerator( new ActGen_MaxActionChainLengthFilter
                     ( new ActGen_StrictCheckPass(), 1 ) );

    //
    // cross
    //
    g->addGenerator( new ActGen_MaxActionChainLengthFilter
                     ( new ActGen_Cross(), 1 ) );

    //
    // direct pass
    //
    // g->addGenerator( new ActGen_RangeActionChainLengthFilter
    //                  ( new ActGen_DirectPass(),
    //                    2, ActGen_RangeActionChainLengthFilter::MAX ) );

    //
    // short dribble
    //
    g->addGenerator( new ActGen_MaxActionChainLengthFilter
                     ( new ActGen_ShortDribble(), 1 ) );

    //
    // self pass (long dribble)
    //
    g->addGenerator( new ActGen_MaxActionChainLengthFilter
                     ( new ActGen_SelfPass(), 1 ) );

    //
    // simple dribble
    //
    // g->addGenerator( new ActGen_RangeActionChainLengthFilter
    //                  ( new ActGen_SimpleDribble(),
    //                    2, ActGen_RangeActionChainLengthFilter::MAX ) );

    return ActionGenerator::ConstPtr( g );
}

/*

    //A. Side of BH

    if(true){
        if(AreSamePoints(MyPos, BHupvert, 1)){
            lastRole = "UV";
            if(!IsOccupied(agent, BHfrontup, obuffer)){
                OccupyHole(BHfrontup);
                lastRole = "FU";
                return true;
            }
        }
        if(AreSamePoints(MyPos, BHdownvert, 1)){
            lastRole = "DV";
            if(!IsOccupied(agent, BHfrontdown, obuffer)){
                OccupyHole(BHfrontdown);
                lastRole = "FD";
                return true;
            }
        }

        //B. Just behind BH
        if(AreSamePoints(MyPos, BHbackup, 1)){
            lastRole = "BU";
            if(!IsOccupied(agent, BHupvert, obuffer)){
                OccupyHole(BHupvert);
                lastRole = "UV";
                return true;
            }
        }
        if(AreSamePoints(MyPos, BHbackdown, 1)){
            lastRole = "BD";
            if(!IsOccupied(agent, BHdownvert, obuffer)){
                OccupyHole(BHdownvert);
                lastRole = "DV";
                return true;
            }
        }

        //C. Far behind
        if(AreSamePoints(MyPos, BHupbackhor, 1)){
            lastRole = "UBH";
            if(!IsOccupied(agent, BHbackup, obuffer)){
                OccupyHole(BHbackup);
                lastRole = "BU";
                return true;
            }
        }
        if(AreSamePoints(MyPos, BHdownbackhor, 1)){
            lastRole = "DBH";
            if(!IsOccupied(agent, BHbackdown, obuffer)){
                OccupyHole(BHbackdown);
                lastRole = "BD";
                return true;
            }
        }
        if(AreSamePoints(MyPos, BHbackhor, 1)){
            lastRole = "BH";
            if(!IsOccupied(agent, BHbackup, obuffer)&&!IsOccupied(agent, BHupbackhor, obuffer)){
                OccupyHole(BHbackup);
                lastRole = "BU";
                return true;
            }
            if(!IsOccupied(agent, BHbackdown, obuffer)&&!IsOccupied(agent, BHdownbackhor, obuffer)){
                OccupyHole(BHbackdown);
                lastRole = "BD";
                return true;
            }
        }
    }
*/