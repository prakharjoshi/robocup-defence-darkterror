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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_basic_offensive_kick.h"

#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/body_dribble.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/body_pass.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>

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

#include <rcsc/common/basic_client.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>
// #include <rcsc/common/free_message_parser.h>

#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>

 #include "body_force_shoot.h"


using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicOffensiveKick::execute( PlayerAgent * agent )
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

    if ( nearest_opp_dist < 7.0 )
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
    if ( nearest_opp_dist > 5.0 )
    {
        Body_Dribble( drib_target,
                      1.0,
                      ServerParam::i().maxDashPower() * 0.4,
                      1
                      ).execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    // opp is near

    // can pass
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
                      ServerParam::i().maxDashPower() * 0.2,
                      1
                      ).execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    if ( nearest_opp_dist > 2.5 )
    {
        Body_HoldBall().execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    {
        Body_AdvanceBall().execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
    }

    return true;

}
