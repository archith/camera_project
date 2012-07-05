#!/bin/sh


export "CONNECTION_FD=8"

#export "QUERY_STRING=name=HttpEventServer%202&type=HTTP&url=http%3A%2F%2F192.168.3.23%3A3200"

#export "QUERY_STRING=name=MotionDetectEvent%203&position=0"

export "QUERY_STRING=event=MotionDetectEvent%203&message=Message%20action%20for%20camera%20(%25C)%20on%20%25D%20-%20%25T&name=MessageAction2 3&server=HttpEventServer%202"