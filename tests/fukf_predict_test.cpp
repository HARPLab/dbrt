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
 * @date 2014
 * @author Jan Issac (jan.issac@gmail.com)
 * @author Manuel Wuthrich (manuel.wuthrich@gmail.com)
 * Max-Planck-Institute for Intelligent Systems,
 * University of Southern California
 */

#include <gtest/gtest.h>

#include <Eigen/Dense>

#include <cmath>
#include <iostream>
#include <vector>
#include <ctime>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/range.hpp>
#include <boost/make_shared.hpp>

#include "fukf_stub_models.hpp"

class FukfTest:
        public testing::Test
{
public:
    typedef Eigen::Matrix<double, 3, 1> State_a;
    typedef Eigen::Matrix<double, 1, 1> State_b_i;

    typedef ProcessModelStub<State_a> ProcessModel_a;
    typedef ProcessModelStub<State_b_i> ProcessModel_b_i;
    typedef ObservationModelStub<State_a, State_b_i> ObservationModel_y_i;

    typedef typename ObservationModel_y_i::Measurement Measurement_y_i;

    typedef ff::FactorizedUnscentedKalmanFilter<
                    ProcessModel_a,
                    ProcessModel_b_i,
                    ObservationModel_y_i,
                    1> Filter;

    FukfTest():
        filter(Filter(boost::make_shared<ProcessModel_a>(),
                      boost::make_shared<ProcessModel_b_i>(),
                      boost::make_shared<ObservationModel_y_i>()))
    {

    }

protected:
    Filter filter;
};

const double EPSILON = 1.0e-12;

TEST_F(FukfTest, predict)
{
    Filter::StateDistribution state_prior;
    Filter::StateDistribution state_predicted;
    state_prior.a_ = State_a::Random();
    state_prior.cov_aa_ = Filter::StateDistribution::CovAA::Random();
    state_prior.cov_aa_ *= state_prior.cov_aa_.transpose();

//    std::cout << state_prior.a_ << std::endl;
//    std::cout << state_prior.cov_aa_ << std::endl;
    for (auto& partition: state_prior.joint_partitions_)
    {
        partition.y = Measurement_y_i::Random();
        partition.b = State_b_i::Random();
        partition.cov_bb = Filter::StateDistribution::CovBB::Random();
        partition.cov_bb *= partition.cov_bb.transpose();

//        std::cout << partition.b << std::endl;
//        std::cout << partition.cov_bb << std::endl;
    }

    filter.predict(state_prior, state_predicted);

//    std::cout << state_predicted.a_ << std::endl;
//    std::cout << state_predicted.cov_aa_ << std::endl;
//    for (auto& partition: state_predicted.joint_partitions_)
//    {
//        std::cout << partition.b << std::endl;
//        std::cout << partition.cov_bb << std::endl;
//    }

    EXPECT_TRUE(state_prior.a_.isApprox(state_predicted.a_));
    EXPECT_TRUE(state_prior.cov_aa_.isApprox(state_predicted.cov_aa_));

    EXPECT_TRUE(state_prior.joint_partitions_[0].b.isApprox(state_predicted.joint_partitions_[0].b));
    EXPECT_TRUE(state_prior.joint_partitions_[0].cov_bb.isApprox(state_predicted.joint_partitions_[0].cov_bb));
}



int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
