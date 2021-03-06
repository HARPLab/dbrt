/*
 * This is part of the Bayesian Object Tracking (bot),
 * (https://github.com/bayesian-object-tracking)
 *
 * Copyright (c) 2015 Max Planck Society,
 * 				 Autonomous Motion Department,
 * 			     Institute for Intelligent Systems
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License License (GNU GPL). A copy of the license can be found in the LICENSE
 * file distributed with this source code.
 */

/**
 * \file gaussian_joint_robot_tracker.cpp
 * \date March 2016
 * \author Jan Issac (jan.issac@gmail.com)
 */

#include <Eigen/Core>
#include <dbrt/tracker/rotary_tracker.h>

namespace dbrt
{
RotaryTracker::RotaryTracker(
    const std::shared_ptr<std::vector<JointFilter>>& joint_filters,
    const std::shared_ptr<KinematicsFromURDF>& kinematics)
    : joint_filters_(joint_filters), kinematics_(kinematics)
{
}

void RotaryTracker::track_callback(const sensor_msgs::JointState& joint_msg)
{
    track(kinematics_->sensor_msg_to_eigen(joint_msg));
}

const std::vector<RotaryTracker::JointBelief>& RotaryTracker::beliefs() const
{
    return beliefs_;
}

std::vector<RotaryTracker::AngleBelief> RotaryTracker::angle_beliefs()
{
    std::vector<AngleBelief> beliefs(beliefs_.size());

    for (int i = 0; i < beliefs.size(); i++)
    {
        beliefs[i].mean(beliefs_[i].mean().middleRows(0, 1));
        beliefs[i].covariance(beliefs_[i].covariance().block(0, 0, 1, 1));
    }

    return beliefs;
}

void RotaryTracker::set_angle_beliefs(
    std::vector<RotaryTracker::AngleBelief> angle_beliefs)
{
    if (beliefs_.size() != angle_beliefs.size())
    {
        std::cout << "your beliefs have the wrong size!" << std::endl;
        exit(-1);
    }

    for (int i = 0; i < beliefs_.size(); i++)
    {
        auto mean = beliefs_[i].mean();
        auto cov = beliefs_[i].covariance();

        // the parameters of the conditional p(b|a) = N(b|Ma + m, C)
        fl::Real M = cov(0, 1) / cov(0, 0);
        fl::Real m = mean(1) - M * mean(0);
        fl::Real C = cov(1, 1) - cov(1, 0) / cov(0, 0) * cov(0, 1);

        auto mean_y = mean;
        auto cov_y = cov;
        mean_y(0) = angle_beliefs[i].mean()(0);
        cov_y(0, 0) = angle_beliefs[i].covariance()(0, 0);

        // put the new marginal and the conditional together to form the joint
        mean_y(1) = M * mean_y(0) + m;
        cov_y(0, 1) = M * cov_y(0, 0);
        cov_y(1, 0) = cov_y(0, 1);
        cov_y(1, 1) = C + M * cov_y(0, 0) * M;

        beliefs_[i].mean(mean_y);
        beliefs_[i].covariance(cov_y);
    }
}

void RotaryTracker::set_beliefs(
    const std::vector<RotaryTracker::JointBelief>& beliefs)
{
    beliefs_ = beliefs;
}

std::vector<RotaryTracker::JointBelief>& RotaryTracker::beliefs()
{
    return beliefs_;
}

RobotTracker::State RotaryTracker::current_state() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return current_state_;
}

/// todo: there should be no obsrv passed in this function
void RotaryTracker::initialize(const std::vector<State>& initial_states)
{
    State state;
    state.resize(joint_filters_->size());

    beliefs_.resize(joint_filters_->size());

    for (int i = 0; i < joint_filters_->size(); ++i)
    {
        beliefs_[i] = (*joint_filters_)[i].create_belief();

        auto cov = beliefs_[i].covariance();
        cov.setZero();
        auto mean = beliefs_[i].mean();
        mean.setZero();
        mean(0) = initial_states[0](i);

        beliefs_[i].mean(mean);
        beliefs_[i].covariance(cov);

        state(i) = beliefs_[i].mean()(0);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    current_state_ = state;
}

auto RotaryTracker::track(const Obsrv& joints_obsrv) -> State
{
    State state;
    state.resize(joint_filters_->size());

    for (int i = 0; i < joint_filters_->size(); ++i)
    {
        (*joint_filters_)[i].predict(
            beliefs_[i], JointInput::Zero(), beliefs_[i]);

        (*joint_filters_)[i].update(
            beliefs_[i], joints_obsrv.middleRows(i, 1), beliefs_[i]);

        state(i) = beliefs_[i].mean()(0);
    }

    //    std::lock_guard<std::mutex> lock(mutex_);
    current_state_ = state;

    return state;
}
}
