/*
 * 吉陵基地
 * RSQ LC 2022-2023 制造
*/

#ifndef DECITHINK_DRIBBLE_H
#define DECITHINK_DRIBBLE_H

#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_action.h>
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

using namespace rcsc;

class Dribble
        : public rcsc::SoccerBehavior{
public:
    Dribble()
    { }
    bool execute(rcsc::PlayerAgent * agent)
    {
        const WorldModel & wm = agent->world();

        const PlayerObject::Cont & opponents = wm.opponentsFromSelf();
        const PlayerObject * nearest_opp = ( opponents.empty()
                                             ? static_cast< PlayerObject * >( 0 )
                                             : opponents.front() );
        const double nearest_opp_dist = ( nearest_opp
                                          ? nearest_opp->distFromSelf()
                                          : 1000.0 );
        const Vector2D nearest_opp_pos = ( nearest_opp
                                           ? nearest_opp->pos()
                                           : Vector2D( -1000.0, 0.0 ) );
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
                    dlog.addText( Logger::TEAM,
                                  __FILE__": (execute) dribble to my body dir" );
                    agent->debugClient().addMessage( "OffKickDrib(1)" );
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

                dlog.addText( Logger::TEAM,
                              __FILE__": (execute) fast dribble to (%.1f, %.1f) max_step=%d",
                              drib_target.x, drib_target.y,
                              max_dash_step );
                agent->debugClient().addMessage( "OffKickDrib(2)" );
                Body_Dribble( drib_target,
                              1.0,
                              ServerParam::i().maxDashPower(),
                              std::min( 5, max_dash_step )
                ).execute( agent );
            }
            else
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (execute) slow dribble to (%.1f, %.1f)",
                              drib_target.x, drib_target.y );
                agent->debugClient().addMessage( "OffKickDrib(3)" );
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
            dlog.addText( Logger::TEAM,
                          __FILE__": opp far. dribble(%.1f, %.1f)",
                          drib_target.x, drib_target.y );
            agent->debugClient().addMessage( "OffKickDrib(4)" );
            Body_Dribble( drib_target,
                          1.0,
                          ServerParam::i().maxDashPower() * 0.4,
                          1
            ).execute( agent );
            agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
            return true;
        }
        if ( nearest_opp_dist > 3.0 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (execute) opp far. dribble(%f, %f)",
                          drib_target.x, drib_target.y );
            agent->debugClient().addMessage( "OffKickDrib(5)" );
            Body_Dribble( drib_target,
                          1.0,
                          ServerParam::i().maxDashPower() * 0.2,
                          1
            ).execute( agent );
            agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
            return true;
        }
        return false;
    }
};


#endif //DECITHINK_DRIBBLE_H
