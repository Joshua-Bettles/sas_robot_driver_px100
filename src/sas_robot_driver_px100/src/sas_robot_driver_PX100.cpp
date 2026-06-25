#include "sas_robot_driver_PX100/sas_robot_driver_PX100.hpp"
#include <iostream>
#include <memory>
#include <sas_core/eigen3_std_conversions.hpp>
#include "dynamixel_sdk/dynamixel_sdk.h"

namespace sas
{


class RobotDriverPX100::Impl
{

public:
    bool connected{false};
    bool motor_on{false};

    VectorXd joint_positions_;
    VectorXd joint_velocities_;
    VectorXd joint_torques_;

    uint16_t goal_position_address = 116;
    uint16_t present_position_address = 132;
    uint16_t goal_velocity_address = 104;
    uint16_t present_velocity_address = 128;
    uint16_t present_effort_address = 126;
    uint16_t torque_on_address = 64;
    uint16_t velocity_limit_address = 44;


    uint16_t data_length_4byte = 4;
    uint16_t data_length_2byte = 2;

    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    bool dxl_addparam_result = false;
    bool dxl_getdata_result = false;
    
    double resolution = 4096.0;
    int velocity_limit = 265;

    double rev_per_min = 0.229;


    std::shared_ptr<dynamixel::PortHandler> portHandler_;
    std::shared_ptr<dynamixel::PacketHandler> packetHandler_;

    std::shared_ptr<dynamixel::GroupSyncWrite> groupSyncWrite_;
    std::shared_ptr<dynamixel::GroupSyncRead>  groupSyncRead_;

    std::shared_ptr<dynamixel::GroupSyncWrite> groupSyncWriteVelocity_;
    std::shared_ptr<dynamixel::GroupSyncRead>  groupSyncReadVelocity_;

    std::shared_ptr<dynamixel::GroupSyncRead>  groupSyncReadTorque_;

    double PI = 3.14159265358979323846;

    Impl()
        {

        }

};

RobotDriverPX100::RobotDriverPX100(const RobotDriverPX100Configuration& configuration, std::atomic_bool* break_loops):
    RobotDriver(break_loops),
    configuration_(configuration)
{
    joint_limits_ = configuration.joint_limits; //for the superclass
    impl_ = std::make_unique<RobotDriverPX100::Impl>();
    impl_->portHandler_ = std::shared_ptr<dynamixel::PortHandler>(dynamixel::PortHandler::getPortHandler(configuration_.usb.c_str())); // your dxl port name
    impl_->packetHandler_ = std::shared_ptr<dynamixel::PacketHandler>(dynamixel::PacketHandler::getPacketHandler(configuration_.protocol)); //protocol version
    impl_->groupSyncWrite_ = std::make_shared<dynamixel::GroupSyncWrite>(
        impl_->portHandler_.get(),
        impl_->packetHandler_.get(),
        impl_->goal_position_address,
        impl_->data_length_4byte
    );

    impl_->groupSyncRead_ = std::make_shared<dynamixel::GroupSyncRead>(
        impl_->portHandler_.get(),
        impl_->packetHandler_.get(),
        impl_->present_position_address,
        impl_->data_length_4byte
    );

    impl_->groupSyncWriteVelocity_ = std::make_shared<dynamixel::GroupSyncWrite>(
        impl_->portHandler_.get(),
        impl_->packetHandler_.get(),
        impl_->goal_velocity_address,
        impl_->data_length_4byte
    );

    impl_->groupSyncReadVelocity_ = std::make_shared<dynamixel::GroupSyncRead>(
        impl_->portHandler_.get(),
        impl_->packetHandler_.get(),
        impl_->present_velocity_address,
        impl_->data_length_4byte
    );

    impl_->groupSyncReadTorque_ = std::make_shared<dynamixel::GroupSyncRead>(
        impl_->portHandler_.get(),
        impl_->packetHandler_.get(),
        impl_->present_effort_address,
        impl_->data_length_4byte
    );
}

RobotDriverPX100::~RobotDriverPX100()
{

}

/**
 * @brief RobotDriverPX100::get_joint_positions
 * This method should always throw an exception if the user
 * tries to obtain the joint positions in an invalid state.
 *
 * One useful way of defining that is with a VectorXd(), which
 * has by default size zero until it is initialized.
 *
 * @return a VectorXd representing the configuration space in radians.
 */
VectorXd RobotDriverPX100::get_joint_positions()
{
    VectorXd temp_joint_position_(configuration_.ids.size());

    for (size_t i = 0; i < configuration_.ids.size(); ++i) {
        auto id = configuration_.ids[i];

        impl_->dxl_comm_result = impl_->groupSyncRead_->txRxPacket();
        if (impl_->dxl_comm_result != COMM_SUCCESS) {
            throw std::runtime_error(impl_->packetHandler_->getTxRxResult(impl_->dxl_comm_result));
        }

        impl_->dxl_getdata_result = impl_->groupSyncRead_->isAvailable(
            id, impl_->present_position_address, impl_->data_length_4byte
        );
        if (!impl_->dxl_getdata_result) {
            throw std::runtime_error(
                "[ID:" + std::to_string(static_cast<int>(id)) +
                "] groupSyncRead getData failed"
            );
        }

        // Assign the value by index
        temp_joint_position_(i) = (impl_->groupSyncRead_->getData(
            id, impl_->present_position_address, impl_->data_length_4byte
        )/impl_->resolution)*2*impl_->PI;
    }

    // Copy to member
    impl_->joint_positions_ = temp_joint_position_;
    return impl_->joint_positions_;

}

/**
 * @brief RobotDriverPX100::set_target_joint_positions
 * This method expects the desired joint positions in radians. The most basic
 * check is for the correct
 *
 * @param desired_joint_positions_rad
 */
void RobotDriverPX100::set_target_joint_positions(const VectorXd &desired_joint_positions_rad)
{
    if (desired_joint_positions_rad.size() != configuration_.ids.size())
        throw std::runtime_error("Incorrect vector size in RobotDriverPX100::set_target_joint_positions");

    for (size_t i = 0; i < desired_joint_positions_rad.size(); i++) {
        auto id = configuration_.ids[i];

        int target_position = (desired_joint_positions_rad(i) /(2*impl_->PI)) * impl_->resolution;

        uint8_t param_goal_position[4];
        param_goal_position[0] = DXL_LOBYTE(DXL_LOWORD(target_position));
        param_goal_position[1] = DXL_HIBYTE(DXL_LOWORD(target_position));
        param_goal_position[2] = DXL_LOBYTE(DXL_HIWORD(target_position));
        param_goal_position[3] = DXL_HIBYTE(DXL_HIWORD(target_position));

        impl_->dxl_addparam_result = impl_->groupSyncWrite_->addParam(id, param_goal_position);
        if (!impl_->dxl_addparam_result) {
            throw std::runtime_error("[ID:" + std::to_string(id) + "] position group write failed");
        }
    }

    impl_->dxl_comm_result = impl_->groupSyncWrite_->txPacket();
    if (impl_->dxl_comm_result != COMM_SUCCESS) {
      throw std::runtime_error(impl_->packetHandler_->getTxRxResult(impl_->dxl_comm_result));
    }
    impl_->groupSyncWrite_->clearParam();
}


/**
 * @brief RobotDriverPX100::get_joint_velocities
 * This method should always throw an exception if the user
 * tries to obtain the joint positions in an invalid state.
 *
 * One useful way of defining that is with a VectorXd(), which
 * has by default size zero until it is initialized.
 *
 * @return a VectorXd representing the configuration space in radians.
 */
VectorXd RobotDriverPX100::get_joint_velocities()
{
    VectorXd temp_joint_velocities_(configuration_.ids.size());

    // Read all velocities at once
    impl_->dxl_comm_result = impl_->groupSyncReadVelocity_->txRxPacket();
    if (impl_->dxl_comm_result != COMM_SUCCESS)
        throw std::runtime_error(impl_->packetHandler_->getTxRxResult(impl_->dxl_comm_result));

    for (size_t i = 0; i < configuration_.ids.size(); ++i) {
        auto id = configuration_.ids[i];

        if (!impl_->groupSyncReadVelocity_->isAvailable(id, impl_->present_velocity_address, impl_->data_length_4byte))
            throw std::runtime_error("[ID:" + std::to_string(id) + "] velocity getData failed");

        uint16_t raw_vel = impl_->groupSyncReadVelocity_->getData(id, impl_->present_velocity_address, impl_->data_length_4byte);
        int16_t signed_vel = *reinterpret_cast<int16_t*>(&raw_vel); // convert to signed
        double rad_per_sec = signed_vel * impl_->rev_per_min * 2*impl_->PI/60.0; // RPM -> rad/s
        temp_joint_velocities_(i) = rad_per_sec;
    }

    impl_->joint_velocities_ = temp_joint_velocities_;
    return impl_->joint_velocities_;
}

/**
 * @brief RobotDriverPX100::set_target_joint_velocities
 * This method expects the desired joint positions in radians. The most basic
 * check is for the correct
 *
 * @param set_target_joint_velocities
 */
void RobotDriverPX100::set_target_joint_velocities(const VectorXd &set_target_joint_velocities)
{
    if (set_target_joint_velocities.size() != configuration_.ids.size())
        throw std::runtime_error("Incorrect vector size in RobotDriverPX100::set_target_joint_positions");

    for (size_t i = 0; i < set_target_joint_velocities.size(); i++) {
        auto id = configuration_.ids[i];

        int target_velocity = (set_target_joint_velocities(i) * (60/(2*impl_->PI))) / impl_->rev_per_min;

        uint8_t param_goal_velocity[4];
        param_goal_velocity[0] = DXL_LOBYTE(DXL_LOWORD(target_velocity));
        param_goal_velocity[1] = DXL_HIBYTE(DXL_LOWORD(target_velocity));
        param_goal_velocity[2] = DXL_LOBYTE(DXL_HIWORD(target_velocity));
        param_goal_velocity[3] = DXL_HIBYTE(DXL_HIWORD(target_velocity));

        impl_->dxl_addparam_result = impl_->groupSyncWriteVelocity_->addParam(id, param_goal_velocity);
        if (!impl_->dxl_addparam_result) {
            throw std::runtime_error("[ID:" + std::to_string(id) + "] position group write failed");
        }
    }

    impl_->dxl_comm_result = impl_->groupSyncWriteVelocity_->txPacket();
    if (impl_->dxl_comm_result != COMM_SUCCESS) {
      throw std::runtime_error(impl_->packetHandler_->getTxRxResult(impl_->dxl_comm_result));
    }
    impl_->groupSyncWriteVelocity_->clearParam();
}


/**
 * @brief RobotDriverPX100::get_joint_torques
 * This method should always throw an exception if the user
 * tries to obtain the joint positions in an invalid state.
 *
 * One useful way of defining that is with a VectorXd(), which
 * has by default size zero until it is initialized.
 *
 * @return a VectorXd representing the configuration space in radians.
 */
VectorXd RobotDriverPX100::get_joint_torques()
{
    VectorXd temp_joint_torques_(configuration_.ids.size());

    // Read all torques at once
    impl_->dxl_comm_result = impl_->groupSyncReadTorque_->txRxPacket();
    if (impl_->dxl_comm_result != COMM_SUCCESS)
        throw std::runtime_error(impl_->packetHandler_->getTxRxResult(impl_->dxl_comm_result));

    for (size_t i = 0; i < configuration_.ids.size(); ++i) {
        auto id = configuration_.ids[i];

        if (!impl_->groupSyncReadTorque_->isAvailable(id, impl_->present_effort_address, impl_->data_length_2byte))
            throw std::runtime_error("[ID:" + std::to_string(id) + "] torque getData failed");

        uint16_t raw_effort = impl_->groupSyncReadTorque_->getData(id, impl_->present_effort_address, impl_->data_length_2byte);
        int16_t signed_effort = *reinterpret_cast<int16_t*>(&raw_effort); // convert to signed
        double effort_percent = (signed_effort / 1023.0) * 100.0; // -100 → 100%
        temp_joint_torques_(i) = effort_percent;
    }

    impl_->joint_torques_ = temp_joint_torques_;
    return impl_->joint_torques_;

}


/**
 * @brief RobotDriverPX100::connect
 *
 * Usually this method will connect to a given ip address. It is also common
 * for this part of the code to stop running programs or turn the robot off.
 * This function is expected to throw an exception of something goes wrong.
 * For instance, if the connection is not established an exception MUST
 * be thrown.
 */
void RobotDriverPX100::connect()
{
    //An example of exception to throw.
    if(impl_->connected)
        throw std::runtime_error("Already connected.");

    if (impl_->portHandler_->openPort()) {
        std::cout<< "Succeeded to open the port to manipulator."<< std::endl;
    } else {
        throw std::runtime_error("Failed to open the port!\n");
    }

    if (impl_->portHandler_->setBaudRate(configuration_.baud_rate)) {
        std::cout<< "Succeeded to change the baudrate." << std::endl;
    } else {
        throw std::runtime_error("Failed to change the baudrate.\n");
    }

    impl_->connected = true;

    impl_->motor_on = false;

}

/**
 * @brief RobotDriverPX100::initialize
 *
 * This method is expected to turn the robot on and initialize the internal joint control loop.
 * If there are any issues, this MUST throw an exception so that the program will finish
 * cleanly.
 *
 * After this method finishes target joint states can be received.
 */
void RobotDriverPX100::initialize()
{

    uint8_t data = 1;
    for(auto id: configuration_.ids){
        impl_->dxl_comm_result = impl_->packetHandler_->write1ByteTxRx(impl_->portHandler_.get(), id, impl_->torque_on_address, data, &impl_->dxl_error);
        if (impl_->dxl_comm_result != COMM_SUCCESS) {
            throw std::runtime_error(impl_->packetHandler_->getTxRxResult(impl_->dxl_comm_result));
        } else if (impl_->dxl_error != 0) {
            throw std::runtime_error(impl_->packetHandler_->getRxPacketError(impl_->dxl_error));
        } else {
            std::cout << "[ID:" << (int)id  <<"] has been successfully connected \n";
        }
    }

    for(auto id: configuration_.ids){
        impl_->dxl_addparam_result = impl_->groupSyncRead_->addParam(id);
        if (!impl_->dxl_addparam_result) {
            throw std::runtime_error(
                "[ID:" + std::to_string(static_cast<int>(id)) +
                "] groupSyncRead addparam failed"
            );
        }

        impl_->dxl_addparam_result = impl_->groupSyncReadVelocity_->addParam(id);
        if (!impl_->dxl_addparam_result) {
            throw std::runtime_error(
                "[ID:" + std::to_string(static_cast<int>(id)) +
                "] groupSyncRead addparam failed"
            );
        }

        impl_->dxl_addparam_result = impl_->groupSyncReadTorque_->addParam(id);
        if (!impl_->dxl_addparam_result) {
            throw std::runtime_error(
                "[ID:" + std::to_string(static_cast<int>(id)) +
                "] groupSyncRead addparam failed"
            );
        }
    }
    impl_->motor_on = true;


}

/**
 * @brief RobotDriverPX100::deinitialize.
 * For safety reasons, this MUST NOT throw exceptions.
 */
void RobotDriverPX100::deinitialize()
{
    uint8_t data = 0;
    for(auto id: configuration_.ids){
        impl_->dxl_comm_result = impl_->packetHandler_->write1ByteTxRx(impl_->portHandler_.get(), id, impl_->torque_on_address, data, &impl_->dxl_error);
        if (impl_->dxl_comm_result != COMM_SUCCESS) {
            throw std::runtime_error(impl_->packetHandler_->getTxRxResult(impl_->dxl_comm_result));
        } else if (impl_->dxl_error != 0) {
            throw std::runtime_error(impl_->packetHandler_->getRxPacketError(impl_->dxl_error));
        } else {
            std::cout << "[ID:" << (int)id  <<"] has been successfully disconnected \n";
        }
    }
    impl_->motor_on = false;

}

/**
 * @brief RobotDriverPX100::disconnect
 * For safety reasons, this MUST NOT throw exceptions.
 */
void RobotDriverPX100::disconnect()
{
    impl_->connected = false;
    impl_->joint_positions_ = VectorXd();
}

}