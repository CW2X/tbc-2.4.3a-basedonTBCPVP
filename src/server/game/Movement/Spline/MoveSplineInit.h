
#ifndef TRINITYSERVER_MOVESPLINEINIT_H
#define TRINITYSERVER_MOVESPLINEINIT_H

#include "MoveSplineInitArgs.h"
#include "PathGenerator.h"
#include "Transport.h"

class Unit;

namespace G3D
{
    class Vector3;
}
enum UnitMoveType : uint8;

namespace Movement
{
    UnitMoveType SelectSpeedType(uint32 moveFlags);

    enum AnimType
    {
        ToGround    = 0, // 460 = ToGround, index of AnimationData.dbc
        FlyToFly    = 1, // 461 = FlyToFly?
        ToFly       = 2, // 458 = ToFly
        FlyToGround = 3  // 463 = FlyToGround
    };

    // Transforms coordinates from global to transport offsets
    class TransportPathTransform
    {
    public:
        TransportPathTransform(Unit* owner, bool transformForTransport)
            : _owner(owner), _transformForTransport(transformForTransport) { }
        G3D::Vector3 operator()(G3D::Vector3 input);

    private:
        Unit* _owner;
        bool _transformForTransport;
    };

    /*  Initializes and launches spline movement
     */
    class MoveSplineInit
    {
    public:

        explicit MoveSplineInit(Unit* m);
        MoveSplineInit(MoveSplineInit&& init) = default;

        /*  Final pass of initialization that launches spline movement.
        Does replace first point with creature current position
         */
        int32 Launch();

        /*  Final pass of initialization that stops movement.
         */
        void Stop();

        /* Adds movement by parabolic trajectory
         * @param amplitude  - the maximum height of parabola, value could be negative and positive
         * @param start_time - delay between movement starting time and beginning to move by parabolic trajectory
         * can't be combined with final animation
         */
        //void SetParabolic(float amplitude, float start_time);
        /* Plays animation after movement done
         * can't be combined with parabolic movement
         */
        //void SetAnimation(AnimType anim);

        /* Adds final facing animation
         * sets unit's facing to specified point/angle after all path done
         * you can have only one final facing: previous will be overriden
         */
        void SetFacing(float angle);
        void SetFacing(G3D::Vector3 const& point);
        void SetFacing(const Unit* target);

        /* Initializes movement by path
         * @param path - array of points, shouldn't be empty
         * @param pointId - Id of fisrt point of the path. Example: when third path point will be done it will notify that pointId + 3 done
         * @param pathTransport - Specify if given path is already on a transport. If given, this function will ensure source unit and target are on the same transport
                                  and assume given point are transport offsets (no further transformation needed)
         */
        void MovebyPath(PointsArray const& path, int32 pointId = 0, Transport* pathTransport = nullptr);

        /* Initializes simple A to B motion, A is current unit's position, B is destination
         */
        void MoveTo(const G3D::Vector3& destination, bool generatePath = true, bool forceDestination = false);
        
        void MoveTo(float x, float y, float z, bool generatePath = true, bool forceDestination = false);

        /* Sets Id of fisrt point of the path. When N-th path point will be done ILisener will notify that pointId + N done
         * Needed for waypoint movement where path splitten into parts
         */
        void SetFirstPointId(int32 pointId) { args.path_Idx_offset = pointId; }

        /* Enables CatmullRom spline interpolation mode(makes path smooth)
         * if not enabled linear spline mode will be choosen. Disabled by default
         */
        //Not BC ? void SetSmooth();
        /* Enables CatmullRom spline interpolation mode, enables flying animation. Disabled by default
         */
        void SetFly();
        /* Enables walk mode. Disabled by default
         */
        void SetWalk(bool enable);
        /* Makes movement cyclic. Disabled by default
         */
        void SetCyclic();
        /* Enables falling mode. Disabled by default
         */
        void SetFall();
#ifdef LICH_KING
        /* Enters transport. Disabled by default
         */
        void SetTransportEnter();
        /* Exits transport. Disabled by default
         */
        void SetTransportExit();
        /* Inverses unit model orientation. Disabled by default
         */
        void SetBackward();
        /* Fixes unit's model rotation. Disabled by default
         */
        void SetOrientationFixed(bool enable);
#endif
        /* Sets the velocity (in case you want to have custom movement velocity)
         * if no set, speed will be selected based on unit's speeds and current movement mode
         * Has no effect if falling mode enabled
         * velocity shouldn't be negative
         */
        void SetVelocity(float velocity);

        PointsArray& Path() { return args.path; }

        /* Disables transport coordinate transformations for cases where raw offsets are available
        */
        void DisableTransportPathTransformations();
    protected:

        MoveSplineInitArgs args;
        Unit*  unit;
    };

    inline void MoveSplineInit::SetFly() { args.flags.EnableFlying(); }
    inline void MoveSplineInit::SetWalk(bool enable) { args.walk = enable; }
    inline void MoveSplineInit::SetCyclic() { args.flags.cyclic = true; }
    inline void MoveSplineInit::SetFall() { args.flags.EnableFalling(); }
    inline void MoveSplineInit::SetVelocity(float vel) { args.velocity = vel; args.HasVelocity = true; }
#ifdef LICH_KING
    inline void MoveSplineInit::SetSmooth() { args.flags.EnableCatmullRom(); }
    inline void MoveSplineInit::SetBackward() { args.flags.backward = true; }
    inline void MoveSplineInit::SetTransportEnter() { args.flags.EnableTransportEnter(); }
    inline void MoveSplineInit::SetTransportExit() { args.flags.EnableTransportExit(); }
    inline void MoveSplineInit::SetOrientationFixed(bool enable) { args.flags.orientationFixed = enable; }
#endif

    inline void MoveSplineInit::MovebyPath(PointsArray const& controls, int32 path_offset /* = 0*/, Transport* pathTransport /*= nullptr*/)
    {
        Transport* currentTransport = unit->GetTransport();
        if (pathTransport)
        {
            //if PathGenerator has a transport, coords are already transport offset (MovementGenerator must respect this)
            args.TransformForTransport = false;  
        }
        if (pathTransport != currentTransport) //always set unit on the same transport as target
        {
            if (currentTransport)
                currentTransport->RemovePassenger(unit);
            if (pathTransport)
                pathTransport->AddPassenger(unit, true);
        }
        args.path_Idx_offset = path_offset;
        args.path.resize(controls.size());
        std::transform(controls.begin(), controls.end(), args.path.begin(), TransportPathTransform(unit, args.TransformForTransport));
    }

    inline void MoveSplineInit::MoveTo(float x, float y, float z, bool generatePath, bool forceDestination)
    {
        MoveTo(G3D::Vector3(x, y, z), generatePath, forceDestination);
    }

#ifdef LICH_KING
    inline void MoveSplineInit::SetParabolic(float amplitude, float time_shift)
    {
        args.time_perc = time_shift;
        args.parabolic_amplitude = amplitude;
        args.flags.EnableParabolic();
    }

    inline void MoveSplineInit::SetAnimation(AnimType anim)
    {
        args.time_perc = 0.f;
        args.flags.EnableAnimation((uint8)anim);
    }
#endif

    inline void MoveSplineInit::SetFacing(G3D::Vector3 const& spot)
    {
        TransportPathTransform transform(unit, args.TransformForTransport);
        G3D::Vector3 finalSpot = transform(spot);
        args.facing.f.x = finalSpot.x;
        args.facing.f.y = finalSpot.y;
        args.facing.f.z = finalSpot.z;
        args.flags.EnableFacingPoint();
    }

    inline void MoveSplineInit::DisableTransportPathTransformations() { args.TransformForTransport = false; }
}
#endif // TRINITYSERVER_MOVESPLINEINIT_H
