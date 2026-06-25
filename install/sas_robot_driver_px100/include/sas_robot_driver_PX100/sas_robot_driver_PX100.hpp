#include <atomic>
#include <memory>
#include <sas_core/sas_robot_driver.hpp>

using namespace Eigen;

namespace sas
{

struct RobotDriverPX100Configuration
{
    std::string usb;
    float protocol;
    int baud_rate;
    std::vector<uint8_t> ids;
    std::tuple<VectorXd,VectorXd> joint_limits;
};

class RobotDriverPX100: public RobotDriver
{
private:
    RobotDriverPX100Configuration configuration_;

    //Use the Impl idiom to "hide" internal driver sources
    class Impl;
    std::unique_ptr<Impl> impl_;

public:

    // Prevent copies as usually drivers have threads
    RobotDriverPX100(const RobotDriverPX100&)=delete;
    RobotDriverPX100()=delete;
    ~RobotDriverPX100();

    // This boilderplate constructor usually does the job well and prevent big changes when
    // parameters change
    RobotDriverPX100(const RobotDriverPX100Configuration &configuration, std::atomic_bool* break_loops);

    /// Everything below this line is an override
    /// the concrete implementations are needed
    VectorXd get_joint_positions() override;
    void set_target_joint_positions(const VectorXd& desired_joint_positions_rad) override;

    VectorXd get_joint_velocities();
    void set_target_joint_velocities(const VectorXd& desired_joint_positions_rad);

    VectorXd get_joint_torques();

    void connect() override;
    void disconnect() override;

    void initialize() override;
    void deinitialize() override;

};
}