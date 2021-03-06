#include <typeinfo>

#include <chrono>
#include <memory>
#include <cstdint>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int16.hpp"
#include "std_msgs/msg/int16_multi_array.hpp"
#include "rexy_msg/msg/leg.hpp"
#include "rexy_msg/msg/leg_list.hpp"
#include <map>
#include <JHPWMDriver/src/JHPWMPCA9685.h>
#include <vector>
#include "yaml-cpp/yaml.h"
using std::placeholders::_1;

using namespace std::chrono_literals;




class MotorControl : public rclcpp::Node
{
private:

  rclcpp::Publisher<rexy_msg::msg::LegList>::SharedPtr publisher;
  rclcpp::Subscription<rexy_msg::msg::LegList>::SharedPtr subscription;
  rexy_msg::msg::LegList goal_state;
  rexy_msg::msg::LegList current_state;
  rclcpp::TimerBase::SharedPtr timer_;
  PCA9685 *pca9685 = new PCA9685();


  std::vector<int16_t> min_pwm_val;
  std::vector<int16_t> max_pwm_val ;
  std::vector<int16_t> min_angle_val;
  std::vector<int16_t> max_angle_val;
  std::vector<float> home_state;

  std::map<std::string, std::vector<int16_t> > name_id_maping;

  float next_step(float goal, float current,float step){

      if (abs(goal-current)>=1){
      if (std::signbit(goal-current)){
        return current - step;
      }
      return current + step;

  }
  return current;
  }

  void read_config(){
    
    YAML::Node config = YAML::LoadFile("/home/rexy/rexy_ws/src/rexy_motor_control/config/rexy_motor_config.yaml");
                   std::cout <<"read configuriton files ... "<<std::endl;

                // loop over the positions Rectangle and print them:
              name_id_maping = config["name_id_maping"].as<std::map<std::string, std::vector<int16_t> >>();
              
              min_pwm_val = config["map_angel_pwm"]["min_pwm_val"].as<std::vector<int16_t>>();
              max_pwm_val = config["map_angel_pwm"]["max_pwm_val"].as<std::vector<int16_t>>();
              min_angle_val = config["map_angel_pwm"]["min_angle_val"].as<std::vector<int16_t>>();
              max_angle_val = config["map_angel_pwm"]["max_angle_val"].as<std::vector<int16_t>>();
              home_state = config["home"]["pos"].as<std::vector<float>>();
  }


  void publish_state()
  {
    for(int leg = 0;leg < 4;leg++){

    std::vector<short> current_id =  this->name_id_maping[this->current_state.legs[leg].name];

    for (int motor_index = 0;motor_index < 3;motor_index++){

      this->current_state.legs[leg].vel[motor_index]=this->goal_state.legs[leg].vel[motor_index];

      this->current_state.legs[leg].pos[motor_index]=this-> next_step(this->goal_state.legs[leg].pos[motor_index],
                                                                      this->current_state.legs[leg].pos[motor_index],
                                                                      this->current_state.legs[leg].vel[motor_index]);

      int pwm_to_motor=convert_agnle_to_pwm(this->current_state.legs[leg].pos[motor_index],current_id[motor_index]);

      this->pca9685->setPWM(current_id[motor_index],
                            0,
                            pwm_to_motor);


      std::cout << this->current_state.legs[leg].name<<" "<<motor_index<<" "<<current_id[motor_index]<<"  "<<
      convert_agnle_to_pwm(this->current_state.legs[leg].pos[motor_index],current_id[motor_index])<<"\t";


    }

    }
        std::cout << std::endl;

    this->publisher->publish(this->current_state);

  }


  rexy_msg::msg::LegList home(){

   // float home_state[3]={90.0, 90.0, 90.0};//, 90.0, 30.0, 60.0, 90.0, 30.0, 60.0, 90.0, 30.0, 60.0};
    rexy_msg::msg::LegList tmp;
    rexy_msg::msg::Leg x;

    for(auto const& val: name_id_maping)
    {
      x.name=val.first;
      x.pos={home_state[0],home_state[1],home_state[2]};
      x.vel={1.0,1.0,1.0};
      tmp.legs.push_back(x);
    }
    return tmp;

  }
  void goal_state_callback(const rexy_msg::msg::LegList::SharedPtr msg)
  {
      for (int i =0; i<4;i++){
      this->goal_state.legs[i]=msg->legs[i];
    }

  }

  int interpolation(double x, int in_min, int in_max, int out_min, int out_max)
{
  return round((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

  int convert_agnle_to_pwm(double angele,int id)
  {
    int min_pwm = std::min(this->min_pwm_val[id],this->max_pwm_val[id]);
    int max_pwm = std::max(this->min_pwm_val[id],this->max_pwm_val[id]);
    
    int goal_pwm=this->interpolation(angele,this->min_angle_val[id],this->max_angle_val[id],this->min_pwm_val[id],this->max_pwm_val[id]);

    if (goal_pwm<min_pwm){
    return min_pwm;
    }
    if (goal_pwm>max_pwm){
    return max_pwm;
    }
    return goal_pwm;
  }

  void stop_motor()
  {
    pca9685->closePCA9685();
  }


public:
 
  MotorControl() : Node("motor_control")
  { 
    this->read_config();

    
    this->goal_state=this->home();

    this->current_state=this->home();

    this->publisher = this->create_publisher<rexy_msg::msg::LegList>("cuurent_state", 10);
    this->subscription = this->create_subscription<rexy_msg::msg::LegList>("goal_state", 10, std::bind(&MotorControl::goal_state_callback, this, _1));

    
    int err = pca9685->openPCA9685();
    //printf("%d\n", err);

    if (err < 0)
    {
      printf("Error: %d", pca9685->error);
      pca9685->closePCA9685();
    }
    else
    {
      pca9685->setAllPWM(0, 0);
      pca9685->reset();
      pca9685->setPWMFrequency(60);
    }
    std::cout<<10<<std::endl;
    this->timer_ = this->create_wall_timer(40ms, std::bind(&MotorControl::publish_state, this));
    
  }
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MotorControl>());
  printf("exit");
  rclcpp::shutdown();
  return 0;
}
