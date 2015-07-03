/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2014 Max-Planck-Institute for Intelligent Systems,
 *                     University of Southern California
 *    Jan Issac (jan.issac@gmail.com)
 *    Manuel Wuthrich (manuel.wuthrich@gmail.com)
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @date 05/25/2014
 * @author Jan Issac (jan.issac@gmail.com)
 * @author Manuel Wuthrich (manuel.wuthrich@gmail.com)
 * Max-Planck-Institute for Intelligent Systems, University of Southern California
 */


#ifndef POSE_TRACKING_MODELS_PROCESS_MODELS_BROWNIAN_OBJECT_MOTION_MODEL_HPP
#define POSE_TRACKING_MODELS_PROCESS_MODELS_BROWNIAN_OBJECT_MOTION_MODEL_HPP

#include <fl/util/assertions.hpp>

#include <pose_tracking/utils/helper_functions.hpp>
#include <pose_tracking/states/free_floating_rigid_bodies_state.hpp>
#include <pose_tracking/models/process_models/damped_wiener_process_model.hpp>
#include <pose_tracking/models/process_models/integrated_damped_wiener_process_model.hpp>

namespace ff
{

// Forward declarations
//TODO: THIS IS REDUNDANT!!
template <typename State, int OBJECTS> class BrownianObjectMotionModel;

namespace internal
{
/**
 * BrownianObjectMotion distribution traits specialization
 * \internal
 */
template <typename State_, int OBJECTS>
struct Traits<BrownianObjectMotionModel<State_, OBJECTS> >
{
    enum
    {
        DIMENSION_PER_OBJECT = 6,
        INPUT_DIMENSION = (OBJECTS == -1) ? -1 : OBJECTS * DIMENSION_PER_OBJECT
    };

    typedef State_                                      State;
    typedef typename State::Scalar                      Scalar;
    typedef Eigen::Matrix<Scalar, INPUT_DIMENSION, 1>   Input;
    typedef Input Noise;

    typedef Eigen::Quaternion<Scalar>                      Quaternion;
    typedef Eigen::Matrix<Scalar, DIMENSION_PER_OBJECT, 1> ObjectState;
    typedef IntegratedDampedWienerProcessModel<ObjectState>     Process;
};
}

/**
 * \class BrownianObjectMotion
 *
 * \ingroup distributions
 * \ingroup process_models
 */
template <typename State_, int OBJECTS = -1>
class BrownianObjectMotionModel
{
public:
    typedef internal::Traits<BrownianObjectMotionModel<State_, OBJECTS> > Traits;

    typedef typename Traits::Scalar     Scalar;
    typedef typename Traits::State      State;
    typedef typename Traits::Input      Input;
    typedef typename Traits::Noise      Noise;
    typedef typename Traits::Quaternion Quaternion;
    typedef typename Traits::Process    Process;

    enum
    {
        DIMENSION_PER_OBJECT = Traits::DIMENSION_PER_OBJECT
    };

public:
    BrownianObjectMotionModel(const double& delta_time,
                              const unsigned& count_objects = OBJECTS):
        state_(count_objects),
        delta_time_(delta_time)
    {
        static_assert_base(State, FreeFloatingRigidBodiesState<OBJECTS>);

        quaternion_map_.resize(count_objects);
        rotation_center_.resize(count_objects);

        for(size_t i = 0; i < count_objects; i++)
        {
            /// \todo check dimensions, not entirely sure about this
            linear_process_.push_back(Process(delta_time, DIMENSION_PER_OBJECT/2));
            angular_process_.push_back(Process(delta_time, DIMENSION_PER_OBJECT/2));
        }

//        linear_process_.resize(count_objects);
//        angular_process_.resize(count_objects);
    }

    virtual ~BrownianObjectMotionModel() { }

    virtual State MapStandardGaussian(const Noise& sample) const
    {
        State new_state(state_.body_count());
        for(size_t i = 0; i < new_state.body_count(); i++)
        {
            Eigen::Matrix<Scalar, 3, 1> position_noise    = sample.template middleRows<3>(i*DIMENSION_PER_OBJECT);
            Eigen::Matrix<Scalar, 3, 1> orientation_noise = sample.template middleRows<3>(i*DIMENSION_PER_OBJECT + 3);
            Eigen::Matrix<Scalar, 6, 1> linear_delta      = linear_process_[i].MapStandardGaussian(position_noise);
            Eigen::Matrix<Scalar, 6, 1> angular_delta     = angular_process_[i].MapStandardGaussian(orientation_noise);

            new_state.position(i) = state_.position(i) + linear_delta.topRows(3);
            Quaternion updated_quaternion(state_.quaternion(i).coeffs() + quaternion_map_[i] * angular_delta.topRows(3));
            new_state.quaternion(updated_quaternion.normalized(), i);
            new_state.linear_velocity(i)  = linear_delta.bottomRows(3);
            new_state.angular_velocity(i) = angular_delta.bottomRows(3);

            // transform to external coordinate system
            new_state.linear_velocity(i) -= new_state.angular_velocity(i).cross(state_.position(i));
            new_state.position(i)        -= new_state.rotation_matrix(i)*rotation_center_[i];
        }

        return new_state;
    }

    virtual void set_delta_time(const double& delta_time)
    {
        delta_time_ = delta_time;
    }


    virtual State state(const State& prev_state,
                        const Noise& noise,
                        const Input& input) const
    {

    }






//    virtual void Condition(const Scalar& delta_time,
//                           const State&  state,
//                           const Input&  control)
//    {
//        state_ = state;
//        for(size_t i = 0; i < state_.body_count(); i++)
//        {
//            quaternion_map_[i] = ff::hf::QuaternionMatrix(state_.quaternion(i).coeffs());

//            // transform the state, which is the pose and velocity with respect to to the origin,
//            // into internal representation, which is the position and velocity of the center
//            // and the orientation and angular velocity around the center
//            state_.position(i)        += state_.rotation_matrix(i)*rotation_center_[i];
//            state_.linear_velocity(i) += state_.angular_velocity(i).cross(state_.position(i));

//            Eigen::Matrix<Scalar, 6, 1> linear_state;
//            linear_state.topRows(3) = Eigen::Vector3d::Zero();
//            linear_state.bottomRows(3) = state_.linear_velocity(i);
//            linear_process_[i].Condition(delta_time,
//                                         linear_state,
//                                         control.template middleRows<3>(i*DIMENSION_PER_OBJECT));

//            Eigen::Matrix<Scalar, 6, 1> angular_state;
//            angular_state.topRows(3) = Eigen::Vector3d::Zero();
//            angular_state.bottomRows(3) = state_.angular_velocity(i);
//            angular_process_[i].Condition(delta_time,
//                                          angular_state,
//                                          control.template middleRows<3>(i*DIMENSION_PER_OBJECT + 3));
//        }
//    }




    virtual void Condition(const State&  state,
                           const Input&  control)
    {
        state_ = state;
        for(size_t i = 0; i < state_.body_count(); i++)
        {
            quaternion_map_[i] = ff::hf::QuaternionMatrix(state_.quaternion(i).coeffs());

            // transform the state, which is the pose and velocity with respect to to the origin,
            // into internal representation, which is the position and velocity of the center
            // and the orientation and angular velocity around the center
            state_.position(i)        += state_.rotation_matrix(i)*rotation_center_[i];
            state_.linear_velocity(i) += state_.angular_velocity(i).cross(state_.position(i));

            Eigen::Matrix<Scalar, 6, 1> linear_state;
            linear_state.topRows(3) = Eigen::Vector3d::Zero();
            linear_state.bottomRows(3) = state_.linear_velocity(i);
            linear_process_[i].Condition(linear_state,
                                         control.template middleRows<3>(i*DIMENSION_PER_OBJECT));

            Eigen::Matrix<Scalar, 6, 1> angular_state;
            angular_state.topRows(3) = Eigen::Vector3d::Zero();
            angular_state.bottomRows(3) = state_.angular_velocity(i);
            angular_process_[i].Condition(angular_state,
                                          control.template middleRows<3>(i*DIMENSION_PER_OBJECT + 3));
        }
    }

    virtual void Parameters(const size_t&                       object_index,
                            const Eigen::Matrix<Scalar, 3, 1>&  rotation_center,
                            const Scalar&                       damping,
                            const typename Process::Operator&   linear_acceleration_covariance,
                            const typename Process::Operator&   angular_acceleration_covariance)
    {
        rotation_center_[object_index] = rotation_center;
        linear_process_[object_index].Parameters(damping, linear_acceleration_covariance);
        angular_process_[object_index].Parameters(damping, angular_acceleration_covariance);
    }

    virtual unsigned InputDimension() const
    {
        return this->NoiseDimension();
    }


    virtual unsigned NoiseDimension() const
    {
        return state_.body_count() * DIMENSION_PER_OBJECT;
    }

    virtual size_t Dimension()
    {
        return state_.rows();
    }


private:
    // conditionals
    State state_;
    std::vector<Eigen::Matrix<Scalar, 4, 3> > quaternion_map_;

    // parameters
    std::vector<Eigen::Matrix<Scalar, 3, 1> > rotation_center_;

    // processes
    std::vector<Process>   linear_process_;
    std::vector<Process>   angular_process_;

    double delta_time_;
};

}

#endif
