#!/usr/bin/env python3

import rospy
import math
import numpy as np

from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseStamped

class NAV2GEO():

    def __init__(self):
        # Set up subscriber and publisher
        self.nav_sub = rospy.Subscriber('camera/odom/sample', Odometry, self.navCallback, queue_size=10)
        self.geo_pub = rospy.Publisher('t265_ned', PoseStamped, queue_size=10)

        # Initialize geo_msg
        self.geo_msg = PoseStamped()

        while not rospy.is_shutdown():
        # wait for new messages and call the callback when they arrive
            rospy.spin()

    def navCallback(self, nav_msg): # TODO: Get Covariances!!!
        # NWU to NED SHOULD BE ALREADY DONE
        self.geo_msg.header = nav_msg.header
        self.geo_msg.pose.position.x = nav_msg.pose.pose.position.x
        self.geo_msg.pose.position.y = nav_msg.pose.pose.position.y
        self.geo_msg.pose.position.z = nav_msg.pose.pose.position.z
        self.geo_msg.pose.orientation.x = nav_msg.pose.pose.orientation.x
        self.geo_msg.pose.orientation.y = nav_msg.pose.pose.orientation.y
        self.geo_msg.pose.orientation.z = nav_msg.pose.pose.orientation.z
        self.geo_msg.pose.orientation.w = nav_msg.pose.pose.orientation.w

        # Publish message
        self.geo_pub.publish(self.geo_msg)

if __name__ == '__main__':
    rospy.init_node('nav2geo', anonymous=True)
    try:
        nav2geo = NAV2GEO()
    except:
        rospy.ROSInterruptException
    pass