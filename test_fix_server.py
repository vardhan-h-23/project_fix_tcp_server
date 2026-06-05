import unittest
import socket

class TestFixServer(unittest.TestCase):

    def setUp(self):
        self.host = 'localhost'
        self.port = 8080

        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.client_socket.settimeout(30)

        self.client_socket.connect((self.host, self.port))

        response  = self.client_socket.recv(1024).decode()
        print(f"Received response on setup: {response}")


    def tearDown(self):
        self.client_socket.close()

    def send_logon_message(self):
        logon_message = "8=FIX.4.2|9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|"

        self.client_socket.sendall(logon_message.encode())

    def do_logon(self):
        self.send_logon_message()

        response = self.client_socket.recv(1024).decode()

    def validate_that_response_is_fix_message(self, response):
        required_tags = ['8', '9', '35', '34', '49', '52', '56', '10']

        response_tags = [tag.split('=')[0] for tag in response.split('|') if '=' in tag]

        for tag in required_tags:
            self.assertIn(tag, response_tags)


def test_logon_message_response():

    test = TestFixServer()

    test.setUp()

    try:
        test.send_logon_message()

        response = test.client_socket.recv(1024).decode()
        response = response.strip()
        expected_response = "8=FIX.4.2|9=69|35=A|34=1|49=SERVER1|52=20260515-12:30:00.000|56=CLIENT1|98=0|108=30|10=215|"

        assert response == expected_response, f"Expected response to be '{expected_response}', but got '{response}'"
        print("Logon message response test passed.")
    finally:
        test.tearDown()

# def test_nos_message_response(): 
#     # This test will depend on the expected response from the server when a NoS message is sent 
#     test_logon_message_response() 
#     nos_fix_message = "8=FIX.4.2|9=138|35=D|34=2|49=CLIENT1|52=20260515-12:35:00.000|56=SERVER1|11=ORD0001|21=1|38=100|40=2|44=245.50|54=1|55=AAPL|59=0|60=20260515-12:35:00.000|10=234|" 
#     TestFixServer().client_socket.sendall(nos_fix_message.encode()) 
#     response = TestFixServer().client_socket.recv(1024).decode() 
#     # we need to match the response tag vias like 39=2|151=0|14=100 
#     # let's write the one on one tag matching for the response validate_that_response_is_fix_message(response) 
#     tags_to_match = {"39": "2", "151": "0", "14": "100"} 
#     response_tags_to_value_map = {} 
#     for tag in response.split('|'): 
#         key, value = tag.split('=') 
#         response_tags_to_value_map[key] = value 
#     for tag, expected_value in tags_to_match.items(): 
#         assert response_tags_to_value_map.get(tag) == expected_value, f"Expected tag {tag} to have value {expected_value}, but got {response_tags_to_value_map.get(tag)}"

def test_nos_message_response():
    test = TestFixServer()

    test.setUp()

    try:
        test.do_logon()

        nos_fix_message = "8=FIX.4.2|9=138|35=D|34=2|49=CLIENT1|52=20260515-12:35:00.000|56=SERVER1|11=ORD0001|21=1|38=100|40=2|44=245.50|54=1|55=AAPL|59=0|60=20260515-12:35:00.000|10=234|"
        test.client_socket.sendall(nos_fix_message.encode())

        response = test.client_socket.recv(1024).decode()
        response = response.strip()
        # print(f"Received response: {response}")
        tags_to_match = {"39": "2", "151": "0", "14": "100"}
        response_tags_to_value_map = {}
        for tag in response.split('|'):
            if '=' in tag:
                key, value = tag.split('=')
                response_tags_to_value_map[key] = value

        for tag, expected_value in tags_to_match.items():
            assert response_tags_to_value_map.get(tag) == expected_value, f"Expected tag {tag} to have value {expected_value}, but got {response_tags_to_value_map.get(tag)}"
        print("NoS message response test passed.")
    finally:
        test.tearDown()

def rejection_scenario1():
    test = TestFixServer()

    test.setUp()

    try:
        # let's test the logon rejection first by sending invalid logon message 
        # 49=CLIENT1 missing and also checksum is wrong in the below message
        invalid_logon_message = "8=FIX.4.2|9=69|35=A|34=1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|" 
        test.client_socket.sendall(invalid_logon_message.encode())
        response = test.client_socket.recv(1024).decode()
        response = response.strip()
        expected_response = "logon validation failed"
        assert response.startswith(expected_response), f"Expected response to be '{expected_response}', but got '{response}'"

        print("Rejection scenario 1 test passed.")

    finally:
        test.tearDown()
def rejection_scenario2():
    test = TestFixServer()

    test.setUp()

    try:
        # 8=Fix version is wrong and also checksum is wrong in the below message
        invalid_logon_message = "9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|"
        test.client_socket.sendall(invalid_logon_message.encode())
        response = test.client_socket.recv(1024).decode()
        response = response.strip()
        expected_response = "Initial Fix Validation failed"
        assert response.startswith(expected_response), f"Expected response to be '{expected_response}', but got '{response}'"

        print("Rejection scenario2 test passed.")

    finally:
        test.tearDown()

# def rejection_scenario3():
#     test = TestFixServer()

#     test.setUp()

#     try:
#         # just put the pipe in the start of the message to make it invalid and also checksum is wrong in the below message
#         invalid_logon_message = "|8=FIX.4.2|9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|"
#         test.client_socket.sendall(invalid_logon_message.encode())
#         response = test.client_socket.recv(1024).decode()
#         response = response.strip()
#         expected_response = "logon validation failed"
#         assert response.startswith(expected_response), f"Expected response to be '{expected_response}', but got '{response}'"

#         print("Rejection scenario3 test passed.")

#     finally:
#         test.tearDown()

def rejection_scenario4():
    test = TestFixServer()

    test.setUp()

    try:
        # put invalid length in the logon message
        invalid_logon_message = "8=FIX.4.2|9=67|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=188|"
        test.client_socket.sendall(invalid_logon_message.encode())
        response = test.client_socket.recv(1024).decode()
        response = response.strip()
        expected_response = "logon validation failed"
        assert response.startswith(expected_response), f"Expected response to be '{expected_response}', but got '{response}'"
        print("Rejection scenario4 test passed.")

    finally:
        test.tearDown()
    
def rejection_scenario5():
    test = TestFixServer()

    test.setUp()

    try:
        nos_fix_message = "8=FIX.4.2|9=138|35=D|34=2|49=CLIENT1|52=20260515-12:35:00.000|56=SERVER1|11=ORD0001|21=1|38=-100|40=2|44=245.50|54=1|55=AAPL|59=0|60=20260515-12:35:00.000|10=234|"
        test.client_socket.sendall(nos_fix_message.encode())

        response = test.client_socket.recv(1024).decode()
        response = response.strip()
        expected_response = "You are not authorised to send NOS"
        assert response.startswith(expected_response), f"Expected response to be '{expected_response}', but got '{response}'"
        print("Rejection scenario5 test passed.")

    finally:
        test.tearDown()


def continuous_parsing_test1():
    test = TestFixServer()
    
    test.setUp()

    try:
        # send two logon messages back to back without waiting for response to test the continuous parsing and also the second message is invalid with wrong checksum
        logon_message1 = "8=FIX.4.2|9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|"
        logon_message2 = "8=FIX.4.2|9=69|35=A|34=1|49=CLIENT2|52=20260515-12:31:00.000|56=SERVER1|98=0|108=30|10=188|" # invalid message with wrong checksum
        combined_message = logon_message1 + logon_message2
        test.client_socket.sendall(combined_message.encode())

        response1 = test.client_socket.recv(1024).decode()
        response1 = response1.strip()
        expected_response1 = "8=FIX.4.2|9=69|35=A|34=1|49=SERVER1|52=20260515-12:30:00.000|56=CLIENT1|98=0|108=30|10=215|" 
        assert response1 == expected_response1, f"Expected response to be '{expected_response1}', but got '{response1}'"

        response2 = test.client_socket.recv(1024).decode()
        response2 = response2.strip()
        # print(f"Received response for second message: {response2}")
        expected_response2 = "Initial Fix Validation failed"
        assert response2.startswith(expected_response2), f"Expected response to be '{expected_response2}', but got '{response2}'"

        print("Continuous parsing test passed.")

    finally:
        test.tearDown()

def continuous_parsing_test2():
    test = TestFixServer()
    
    test.setUp()

    try:
        # send a valid logon message followed by an valid NoS message without waiting for response to test the continuous parsing and also the second message is invalid with wrong checksum
        logon_message = "8=FIX.4.2|9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|"
        nos_fix_message = "8=FIX.4.2|9=138|35=D|34=2|49=CLIENT1|52=20260515-12:35:00.000|56=SERVER1|11=ORD0001|21=1|38=100|40=2|44=245.50|54=1|55=AAPL|59=0|60=20260515-12:35:00.000|10=234|"
        combined_message = logon_message + nos_fix_message
        test.client_socket.sendall(combined_message.encode())
        response1 = test.client_socket.recv(1024).decode()
        response1 = response1.strip()
        expected_response1 = "8=FIX.4.2|9=69|35=A|34=1|49=SERVER1|52=20260515-12:30:00.000|56=CLIENT1|98=0|108=30|10=215|" 
        assert response1 == expected_response1, f"Expected response to be '{expected_response1}', but got '{response1}'"

        response2 = test.client_socket.recv(1024).decode()
        response2 = response2.strip()
        # print(f"Received response for NoS message: {response2}")
        tags_to_match = {"39": "2", "151": "0", "14": "100"}
        response_tags_to_value_map = {}
        for tag in response2.split('|'):
            if '=' in tag:
                key, value = tag.split('=')
                response_tags_to_value_map[key] = value
        for tag, expected_value in tags_to_match.items():
            assert response_tags_to_value_map.get(tag) == expected_value, f"Expected tag {tag} to have value {expected_value}, but got {response_tags_to_value_map.get(tag)}"

        print("Continuous parsing test 2 passed.")
    finally:
        test.tearDown()


def continuous_parsing_test3():
    # let's test the half Fix message scenario where the first message is sent in two parts to test the continuous parsing and also the second message is valid
    test = TestFixServer()

    test.setUp()

    try:
        half_logon_msg = "8=FIX.4.2|9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000"
        remaining_logon_msg = "|56=SERVER1|98=0|108=30|10=189|"
        test.client_socket.sendall(half_logon_msg.encode())
        test.client_socket.sendall(remaining_logon_msg.encode())
        response1 = test.client_socket.recv(1024).decode()
        response1 = response1.strip()
        expected_response1 = "8=FIX.4.2|9=69|35=A|34=1|49=SERVER1|52=20260515-12:30:00.000|56=CLIENT1|98=0|108=30|10=215|" 
        assert response1 == expected_response1, f"Expected response to be '{expected_response1}', but got '{response1}'"

        print("Continuous parsing test 3 passed.")
    finally:
        test.tearDown()


def continuous_parsing_test4():
    # let's test the scenario where multiple messages are sent in parts to test the continuous parsing and also the second message is valid
    test = TestFixServer()

    test.setUp()

    try:
        logon_message = "8=FIX.4.2|9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|"
        half_nos_msg = "8=FIX.4.2|9=138|35=D|34=2|49=CLIENT1|52=20260515-12:35:00.000|56=SERVER1|11=ORD0001"
        remaining_nos_msg = "|21=1|38=100|40=2|44=245.50|54=1|55=AAPL|59=0|60=20260515-12:35:00.000|10=234|"
        combined_message = logon_message + half_nos_msg
        test.client_socket.sendall(combined_message.encode())
        response1 = test.client_socket.recv(1024).decode()
        response1 = response1.strip()
        expected_response1 = "8=FIX.4.2|9=69|35=A|34=1|49=SERVER1|52=20260515-12:30:00.000|56=CLIENT1|98=0|108=30|10=215|" 
        assert response1 == expected_response1, f"Expected response to be '{expected_response1}', but got '{response1}'"

        test.client_socket.sendall(remaining_nos_msg.encode())
        response2 = test.client_socket.recv(1024).decode()
        response2 = response2.strip()
        # print(f"Received response for NoS message: {response2}")
        tags_to_match = {"39": "2", "151": "0", "14": "100"}
        response_tags_to_value_map = {}
        for tag in response2.split('|'):
            if '=' in tag:
                key, value = tag.split('=')
                response_tags_to_value_map[key] = value
        for tag, expected_value in tags_to_match.items():
            assert response_tags_to_value_map.get(tag) == expected_value, f"Expected tag {tag} to have value {expected_value}, but got {response_tags_to_value_map.get(tag)}"

        print("Continuous parsing test 4 passed.")
    finally:
        test.tearDown()


def continuous_parsing_test5():
    # let's test the scenario where multiple messages are sent in parts to test the continuous parsing and also the second message is invalid with wrong checksum
    test = TestFixServer()

    test.setUp()

    try:
        logon_message = "8=FIX.4.2|9=69|35=A|34=1|8=FIX.4.2|9=69|8=FIX.4.2|9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|"
        test.client_socket.sendall(logon_message.encode())
        response1 = test.client_socket.recv(1024).decode()
        response1 = response1.strip()
        expected_response1 = "8=FIX.4.2|9=69|35=A|34=1|49=SERVER1|52=20260515-12:30:00.000|56=CLIENT1|98=0|108=30|10=215|"
        assert response1 == expected_response1, f"Expected response to be '{expected_response1}', but got '{response1}'"
        print("Continuous parsing test 5 passed.")
    finally:
        test.tearDown()

def continuous_parsing_test6():
    # let's test the scenario where multiple messages are sent in parts to test the continuous parsing and also the second message is invalid with wrong checksum
    test = TestFixServer()

    test.setUp()

    try:
        logon_message = "8=FIX.4.2|9=69|35=A|34=1|49=CLIENT1|52=20260515-12:30:00.000|56=SERVER1|98=0|108=30|10=189|"
        nos_fix_msg1 = "8=FIX.4.2|9=138|35=D|34=2|49=CLIENT1|52=20260515-12:35:00.000|56=SERVER1|11=ORD0001|21=1|38=100|38=100|40=2|44=245.50|54=1|55=AAPL|59=0|60=20260515-12:35:00.000|10=234|"
        nos_fix_msg2 = "8=FIX.4.2|9=138|35=D|34=3|49=CLIENT1|52=20260515-12:35:00.000|56=SERVER1|11=ORD0001|21=1|38=100|40=2|44=245.50|54=1|55=AAPL|59=0|60=20260515-12:35:00.000|10=234|"
        combined_message = logon_message + nos_fix_msg1 + nos_fix_msg2
        test.client_socket.sendall(combined_message.encode())
        response1 = test.client_socket.recv(1024).decode()
        response1 = response1.strip()
        expected_response1 = "8=FIX.4.2|9=69|35=A|34=1|49=SERVER1|52=20260515-12:30:00.000|56=CLIENT1|98=0|108=30|10=215|"
        assert response1 == expected_response1, f"Expected response to be '{expected_response1}', but got '{response1}'"
        # print(f"Received response for first message: {response1}")
        response2 = test.client_socket.recv(1024).decode()
        response2 = response2.strip()
        expected_response2 = "Initial Fix Validation failed"
        # print(f"Received response for second message: {response2}")
        assert response2.startswith(expected_response2), f"Expected response to be '{expected_response2}', but got '{response2}'"
        # response3 = test.client_socket.recv(1024).decode()
        # response3 = response3.strip()
        # print(f"Received response for third message: {response3}")
        tags_to_match = {"39": "2", "151": "0", "14": "100"}
        response_tags_to_value_map = {}
        for tag in response2.split('|'):
            if '=' in tag:
                key, value = tag.split('=')
                response_tags_to_value_map[key] = value
        for tag, expected_value in tags_to_match.items():
            assert response_tags_to_value_map.get(tag) == expected_value, f"Expected tag {tag} to have value {expected_value}, but got {response_tags_to_value_map.get(tag)}"
        print("Continuous parsing test 6 passed.")


    finally:
        test.tearDown()


if __name__ == '__main__':
    test_logon_message_response()
    test_nos_message_response()
    rejection_scenario1()
    rejection_scenario2()
    # # rejection_scenario3() // it no loger is valid because the next fix message will start from 8=FIX...
    rejection_scenario4()
    rejection_scenario5()
    continuous_parsing_test1()
    continuous_parsing_test2()
    continuous_parsing_test3()
    continuous_parsing_test4()
    continuous_parsing_test5()
    continuous_parsing_test6()