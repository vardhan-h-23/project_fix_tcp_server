// I am keeping things simple as of now about the fix mesage
// 8=FIX.4.2|9=112|35=A|34=1|49=CLIENT1|52=20260513-13:45:00.000|56=SERVER1|98=0|108=30|10=185|
// assume that we will be receiving this

/// Validtation params
// 8	BeginString	FIX version
// 9	BodyLength	Message integrity
// 35	MsgType	Must be A for Logon
// 10	CheckSum	Message integrity
#pragma once

#include "controller.h"

Fix_Parser parser;
void controller::OnMessageReceive(Fix_Message &msg, tcp_connection::pointer conn)
{
    for (auto x : msg.msg)
    {
        std::cout << x.first << " " << x.second << "\n";
    }
    if (!msg.has_field(35))
    {
        std::cout << "Invalid message * rquired tag not found\n";
        conn->do_write("Invalid message * rquired tag not found\n");
        conn->close();
        return;
    }
    std::string type = msg.get_field(35 /*fix type*/);
    if (type == "A")
    {
        if (conn->state_ == tcp_connection_state::Authenticating)
        {
            if (validate_logon(msg))
            {
                conn->state_ = tcp_connection_state::Connected;
                std::cout << "logon successful\n";
                std::swap(msg.msg[49], msg.msg[56]);
                conn->do_write(parser.parse_to_string(msg) + "\n");
            }
            else
            {
                std::cout << "logon failed\n";
                conn->do_write("logon validation failed\n");
                conn->close();
            }
        }
        else
        {
            std::cout << "already authenticated\n";
            // TBD what exactly we need to do
            //  conn->close();
        }
    }
    else if (type == "D")
    {
        if (conn->state_ != tcp_connection_state::Connected)
        {
            conn->do_write("You are not authorised to send NOS\n");
            conn->close();
        }
        // TBD
        if (!validate_nos(msg))
        {
            conn->do_write("NOS params are invalid\n");
        }
        else
        {
            // 35 D -> 35=8, 37 OrderID ==uuid, 39 OrdStatus=filled, 151 LeavesQty = 0, 14 CumQty = 38, 60 TransactTime, 6=0
            // 49 -> 56 swap
            boost::uuids::uuid uuid = boost::uuids::random_generator()();
            msg.seq.pop_back();
            msg.add_field(35, "8");
            msg.add_field(34, std::to_string(msg.get_int(34)+1));
            msg.add_field(37, boost::uuids::to_string(uuid));
            msg.seq.push_back(37);
            msg.add_field(39, "2");
            msg.seq.push_back(39);
            msg.add_field(151, "0");
            msg.seq.push_back(151);
            msg.add_field(14, msg.get_field(38));
            msg.seq.push_back(14);
            std::swap(msg.msg[49], msg.msg[56]);
            msg.add_field(60, getFormattedTime());
            msg.seq.push_back(60);
            msg.seq.push_back(10);
            msg.add_field(52, getFormattedTime());
            conn->do_write(parser.parse_to_string(msg) + "\n");
        }
    }
    else
    {
    }
}

bool controller::validate_logon(Fix_Message &msg)
{
    if (!msg.has_field(8) || msg.get_field(8) != "FIX.4.2")
    {
        return false;
    }
    // TBD is client validation etc.. now allowing all the connections
    if (!msg.has_field(49))
    {
        return false;
    }
    // checksum is diff
    // active_connections[msg.msg[49]]++;
    // if (!msg.has_field(34) || msg.get_int(34) != active_connections[msg.msg[49]])
    // {
    //     return false;
    // }
    return true;
}

bool controller::validate_nos(Fix_Message &msg)
{
    return true;
}
// 8=FIX.4.2|9=178|35=D|34=2|49=CLIENT1|52=20260514-12:30:15.123|56=SERVER1|11=ORD000001|21=1|38=100|40=2|44=245.50|54=1|55=AAPL|59=0|60=20260514-12:30:15.123|10=128|
// 8=FIX.4.2|9=210|35=8|34=3|49=SERVER1|52=20260514-12:30:15.200|56=CLIENT1|6=0|11=ORD000001|14=0|17=EXEC000001|20=0|31=0|32=0|37=EXCHORD0001|38=100|39=0|54=1|55=AAPL|150=0|151=100|60=20260514-12:30:15.200|10=092|

//
// Tag,Tag Name,Value,Explanation
// 8,BeginString,FIX.4.2,Defines the FIX protocol version being used.
// 9,BodyLength,210,The number of bytes in the message from tag 35 up to tag 10.
// 35,MsgType,8,Defines the message type. 8 stands for Execution Report.
// 34,MsgSeqNum,3,The sequence number of this message (this is the 3rd message sent in this session).
// 49,SenderCompID,SERVER1,The ID of the party sending the message (likely the broker or exchange).
// 56,TargetCompID,CLIENT1,The ID of the party receiving the message (the trader or client system).
// 52,SendingTime,20260514-12:30:15.200,The UTC timestamp of when this message was transmitted.

// Tag,Tag Name,Value,Explanation
// 11,ClOrdID,ORD000001,Client Order ID: The unique identifier assigned to this order by the client.
// 37,OrderID,EXCHORD0001,Exchange Order ID: The unique identifier assigned to this order by the server/exchange.
// 17,ExecID,EXEC000001,Execution ID: A unique identifier for this specific execution report/transaction.
// 20,ExecTransType,0,0 means New. It indicates this is a new execution report (not a cancellation or correction). Note: This tag is specific to FIX 4.2 and was deprecated in FIX 4.3+.
// 150,ExecType,0,0 means New. Describes the specific event causing this report (the order was accepted).
// 39,OrdStatus,0,0 means New. The current state of the order. It has been accepted but no trades have happened yet.
// 55,Symbol,AAPL,The ticker symbol of the financial instrument being traded (Apple Inc.).
// 54,Side,1,1 means Buy. (A value of 2 would mean Sell).
// 38,OrderQty,100,The total number of shares requested in the original order.
// 151,LeavesQty,100,"The number of shares left to execute. Since nothing has filled yet, all 100 shares remain."
// 14,CumQty,0,Cumulative Quantity: The total number of shares filled so far.
// 6,AvgPx,0,"Average Price: The average price of the filled shares. Since CumQty is 0, this is also 0."
// 32,LastShares,0,The quantity of shares bought/sold in the last execution. (0 because no fill happened).
// 31,LastPx,0,The price at which the last execution occurred. (0 because no fill happened).
// 60,TransactTime,20260514-12:30:15.200,The UTC timestamp of when the transaction/event actually occurred at the exchange.