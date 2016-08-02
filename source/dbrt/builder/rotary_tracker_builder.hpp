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
 * \file particle_tracker_builder.hpp
 * \date November 2015
 * \author Jan Issac (jan.issac@gmail.com)
 */

#pragma once

#include <exception>

#include <dbot/object_resource_identifier.hpp>
#include <dbot/tracker/object_tracker.hpp>
#include <dbot/builder/particle_tracker_builder.hpp>

#include <dbrt/kinematics_from_urdf.h>
#include <dbrt/tracker/rotary_tracker.h>
#include <dbrt/builder/factorized_transition_builder.hpp>
#include <dbrt/builder/rotary_sensor_builder.hpp>

namespace dbrt
{
template <typename Tracker>
class RotaryTrackerBuilder
{
public:
    typedef typename Tracker::State State;
    typedef typename Tracker::Noise Noise;
    typedef typename Tracker::Input Input;
    typedef typename Tracker::JointFilter JointFilter;

public:
    RotaryTrackerBuilder(
        const int& joint_count,
        const std::vector<int> joint_order,
        const std::shared_ptr<FactorizedTransitionBuilder<Tracker>>&
            transition_builder,
        const std::shared_ptr<RotarySensorBuilder<Tracker>>& sensor_builder)
        : joint_count_(joint_count),
          joint_order_(joint_order),
          transition_builder_(transition_builder),
          sensor_builder_(sensor_builder)
    {
    }

    /**
     * \brief Builds the Rbc PF tracker
     */
    std::shared_ptr<Tracker> build()
    {
        auto joint_filters = create_joint_filters();

        auto tracker = std::make_shared<Tracker>(joint_filters, joint_order_);

        return tracker;
    }

    virtual std::shared_ptr<std::vector<JointFilter>> create_joint_filters()
    {
        auto joint_filters = std::make_shared<std::vector<JointFilter>>();

        for (int i = 0; i < joint_count_; ++i)
        {
            auto transition =
                this->transition_builder_->build(i);
            auto sensor = this->sensor_builder_->build(i);

            auto filter = JointFilter(*transition, *sensor);

            joint_filters->push_back(filter);
        }
        return joint_filters;
    }

protected:
    int joint_count_;
    std::vector<int> joint_order_;
    std::shared_ptr<FactorizedTransitionBuilder<Tracker>> transition_builder_;
    std::shared_ptr<RotarySensorBuilder<Tracker>> sensor_builder_;
};
}
