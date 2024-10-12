#include <micro_ros_arduino.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32_multi_array.h>
#include <std_msgs/msg/float32.h>
// Subscriber Code with optional Feedback Publisher
// ===========================================================
// ESP32 Communication via MicroROS (Publisher -> Subscriber)
// CODE BY: PEDRO H. PERES
// GITHUB: PedroH-Peres (https://github.com/PedroH-Peres/microros_array_communication)
// ABOUT: This is code that is part of the pub_sub set of Float32MultiArray with a feedback topic that sums the Array values.
// OBJECTIVE: It aims to facilitate the understanding of the publisher - subscriber set using standard array type messages
// ===========================================================

rcl_subscription_t subscriber;
rcl_publisher_t publisher;
std_msgs__msg__Float32 feedback_msg;
std_msgs__msg__Float32MultiArray msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

// ===== Config Variables =====
int DATA_SIZE = 3; // Must have the same value as the SIZE Variable in the publisher code
// ============================

#define LED_PIN 13

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}


void error_loop(){
  while(1){
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}

void subscription_callback(const void * msgin)
{  
  const std_msgs__msg__Float32MultiArray * msg = (const std_msgs__msg__Float32MultiArray *)msgin;
  float sum = 0.0;
  for(size_t i = 0; i < msg->data.size; i++){
    sum += msg->data.data[i];
  }
  feedback_msg.data = sum;
  RCSOFTCHECK(rcl_publish(&publisher, &feedback_msg, NULL));
  sum = 0.0;
}

void setup() {
  set_microros_transports();
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  
  
  delay(2000);

  allocator = rcl_get_default_allocator();

  //create init_options
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, "micro_ros_node2", "", &support));

  // create publisher
  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "sum_publisher"));

  // create subscriber
  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
    "micro_ros_publisher"));

  msg.data.data = (float *) malloc(DATA_SIZE * sizeof(float)); // Aloca memória para 3 floats
  msg.data.size = DATA_SIZE;
  msg.data.capacity = DATA_SIZE;

  feedback_msg.data = 0.0;

  // create executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));
}

void loop() {
  delay(100);
  RCCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
}
