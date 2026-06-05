I accomplished the target / tests set for this slot
Let me clear the choices I made; 
1. Reading the buffer and processing it and later reading the next upcoming buffer
2. while processing it I am concluding all the fix messages end to end; and leaving the incomplete messages to be completed in next buffer read
3. if buffer contain multiple messages: I am removing the concluded messages after all the messages are processed only leaving the incomplete message in the buffer as removing and replacing the leftover buffer looks costly 
4. For receiving the leftover messages I am placing the remaining message in the starting of buffer and then putting the upcoming leftover just after that 
5. If any incorrect message is found after the logon i am just replying as usual and moving on to the next message present in buffer without breaking the TCP session
6. i created/added more tests for scenearios names as continuous_parsing_test in the test_fix_server
    a. replying 2 continuous message (one is right other is wrong logon)
    b. replying 2 continuous message (one is logon and other is NOS)
    c. first is half logon followed by remaining 
    d. first is complete logon + half NOS followed buffer is remaining half NOS
    e. logon message which consists of various currpt messages befor valid message and I moved to the last '8=FIX' flag just before the first '|10=' present in the buffer 
    f. message consists of logon + incorrect NOS(due to duplicate param in message bidy) + correct NOS

7. I am basically discarding the leftovers buffer which is not between consecutive 8=FIX and |10= flag and only consider that specific buffer as message body and rejecting/failing any message which consists of duplicate flag or parameter; its good to reject because the validation of body length and checksum may fail