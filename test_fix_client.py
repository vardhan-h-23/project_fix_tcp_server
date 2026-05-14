#!/usr/bin/env python3
"""
FIX 4.2 Test Client for C++ TCP Server
Tests the complete flow: Logon -> ExecutionReport for NewOrderSingle
"""

import socket
import time
import logging
from datetime import datetime
from typing import Optional, Dict, Tuple

# Configure logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/tmp/fix_client.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)


class FIXParser:
    """Parser for FIX protocol messages using pipe delimiter"""

    DELIMITER = '|'

    @staticmethod
    def calculate_checksum(message: str) -> int:
        """Calculate FIX checksum as sum of bytes mod 256"""
        return sum(ord(c) for c in message) % 256

    @staticmethod
    def calculate_body_length(body: str) -> int:
        """Calculate body length from tag 35 to before tag 10"""
        return len(body)

    @staticmethod
    def parse_message(msg_str: str) -> Optional[Dict[int, str]]:
        """Parse FIX message string into tag-value dict"""
        try:
            fields = {}
            # Split by pipe and process each tag=value pair
            parts = msg_str.strip().split(FIXParser.DELIMITER)
            for part in parts:
                if not part:
                    continue
                if '=' not in part:
                    continue
                tag_str, value = part.split('=', 1)
                try:
                    tag = int(tag_str)
                    fields[tag] = value
                except ValueError:
                    continue
            return fields if fields else None
        except Exception as e:
            logger.error(f"Error parsing message: {e}")
            return None

    @staticmethod
    def format_message(fields: Dict[int, str], tag_order: list = None) -> str:
        """Format fields into FIX message string"""
        if tag_order is None:
            tag_order = sorted(fields.keys())

        # Build body (everything except BeginString, BodyLength, and Checksum)
        body_parts = []
        for tag in tag_order:
            if tag not in [8, 9, 10]:
                body_parts.append(f"{tag}={fields[tag]}")

        body = FIXParser.DELIMITER.join(body_parts)
        if body:
            body += FIXParser.DELIMITER

        body_length = len(body)

        # Calculate checksum on full message body
        checksum_body = f"35={fields.get(35, '')}|" if 35 in fields else ""
        checksum_str = FIXParser.DELIMITER.join(
            [f"{tag}={fields[tag]}" for tag in tag_order if tag not in [8, 9, 10]]
        )
        checksum = FIXParser.calculate_checksum(checksum_str)

        # Construct complete message
        message = f"8={fields.get(8, 'FIX.4.2')}|9={body_length}|"
        message += body
        message += f"10={checksum:03d}|"

        return message


class FIXClient:
    """FIX Protocol Client for testing"""

    def __init__(self, host: str = 'localhost', port: int = 8080):
        self.host = host
        self.port = port
        self.socket = None
        self.sequence_num = 0

    def connect(self) -> bool:
        """Connect to FIX server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            logger.info(f"Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            logger.error(f"Connection failed: {e}")
            return False

    def disconnect(self):
        """Disconnect from server"""
        if self.socket:
            self.socket.close()
            logger.info("Disconnected")

    def send_raw(self, message: str) -> bool:
        """Send raw message bytes"""
        try:
            data = message.encode('utf-8')
            self.socket.send(data)
            logger.info(f"SENT: {message.strip()}")
            logger.debug(f"RAW BYTES: {data.hex()}")
            return True
        except Exception as e:
            logger.error(f"Send failed: {e}")
            return False

    def receive(self, timeout: float = 5.0) -> Optional[str]:
        """Receive message from server"""
        try:
            self.socket.settimeout(timeout)
            data = self.socket.recv(10240)
            if data:
                message = data.decode('utf-8')
                logger.info(f"RECEIVED: {message.strip()}")
                logger.debug(f"RAW BYTES: {data.hex()}")
                return message
            return None
        except socket.timeout:
            logger.warning("Receive timeout")
            return None
        except Exception as e:
            logger.error(f"Receive failed: {e}")
            return None

    def get_formatted_time(self) -> str:
        """Get current time in FIX format: YYYYMMDD-HH:MM:SS.sss"""
        now = datetime.now()
        return now.strftime('%Y%m%d-%H:%M:%S.%f')[:-3]

    def send_logon(self, sender_id: str = 'CLIENT1', target_id: str = 'SERVER1') -> bool:
        """Send Logon message (35=A)"""
        self.sequence_num += 1

        fields = {
            8: 'FIX.4.2',
            35: 'A',
            34: str(self.sequence_num),
            49: sender_id,
            56: target_id,
            52: self.get_formatted_time(),
            98: '0',  # EncryptMethod: None
            108: '30'  # HeartBtInt: 30 seconds
        }

        # Build message manually with proper tag order
        msg = f"8=FIX.4.2|"
        msg += f"9=XXX|"  # Placeholder for length
        msg += f"35=A|"
        msg += f"34={self.sequence_num}|"
        msg += f"49={sender_id}|"
        msg += f"52={fields[52]}|"
        msg += f"56={target_id}|"
        msg += f"98=0|"
        msg += f"108=30|"

        # Calculate body length (from tag 35 to before tag 10)
        body_part = f"35=A|34={self.sequence_num}|49={sender_id}|52={fields[52]}|56={target_id}|98=0|108=30|"
        body_len = len(body_part)

        # Calculate checksum
        checksum = FIXParser.calculate_checksum(body_part)

        msg = f"8=FIX.4.2|9={body_len}|{body_part}10={checksum:03d}|"

        return self.send_raw(msg)

    def send_new_order_single(self, symbol: str = 'AAPL', qty: int = 100,
                             price: float = 245.50, side: int = 1) -> bool:
        """Send NewOrderSingle message (35=D)"""
        self.sequence_num += 1
        send_time = self.get_formatted_time()

        # Build NOS message
        msg = f"8=FIX.4.2|"
        msg += f"9=XXX|"  # Placeholder
        msg += f"35=D|"
        msg += f"34={self.sequence_num}|"
        msg += f"49=CLIENT1|"
        msg += f"52={send_time}|"
        msg += f"56=SERVER1|"
        msg += f"11=ORD000001|"  # ClOrdID
        msg += f"21=1|"  # HandlInst: Automated
        msg += f"38={qty}|"  # OrderQty
        msg += f"40=2|"  # OrdType: Limit
        msg += f"44={price}|"  # Price
        msg += f"54={side}|"  # Side: Buy
        msg += f"55={symbol}|"  # Symbol
        msg += f"59=0|"  # TimeInForce: Day
        msg += f"60={send_time}|"  # TransactTime

        # Calculate body length
        body_part = (f"35=D|34={self.sequence_num}|49=CLIENT1|52={send_time}|56=SERVER1|"
                    f"11=ORD000001|21=1|38={qty}|40=2|44={price}|54={side}|55={symbol}|59=0|60={send_time}|")
        body_len = len(body_part)

        # Calculate checksum
        checksum = FIXParser.calculate_checksum(body_part)

        msg = f"8=FIX.4.2|9={body_len}|{body_part}10={checksum:03d}|"

        return self.send_raw(msg)


def validate_logon_response(response: str) -> bool:
    """Validate Logon acknowledgment response"""
    parsed = FIXParser.parse_message(response)
    if not parsed:
        logger.error("Failed to parse logon response")
        return False

    # Check required fields
    if parsed.get(35) != 'A':
        logger.error(f"Expected MsgType 'A', got {parsed.get(35)}")
        return False

    if parsed.get(8) != 'FIX.4.2':
        logger.error(f"Expected BeginString 'FIX.4.2', got {parsed.get(8)}")
        return False

    # Check sender/receiver were swapped
    if parsed.get(49) != 'SERVER1' or parsed.get(56) != 'CLIENT1':
        logger.error(f"Sender/receiver not swapped. Got 49={parsed.get(49)}, 56={parsed.get(56)}")
        return False

    logger.info("✓ Logon response valid")
    return True


def validate_execution_report(response: str, order_qty: int = 100, order_price: float = 245.50) -> bool:
    """Validate ExecutionReport response for NewOrderSingle"""
    parsed = FIXParser.parse_message(response)
    if not parsed:
        logger.error("Failed to parse execution report")
        return False

    # Check message type is ExecutionReport (8)
    if parsed.get(35) != '8':
        logger.error(f"Expected MsgType '8' (ExecutionReport), got {parsed.get(35)}")
        return False

    # Check order status is filled (2)
    if parsed.get(39) != '2':
        logger.error(f"Expected OrdStatus '2' (Filled), got {parsed.get(39)}")
        return False

    # Check LeavesQty is 0
    if parsed.get(151) != '0':
        logger.error(f"Expected LeavesQty '0', got {parsed.get(151)}")
        return False

    # Check CumQty equals order quantity
    if parsed.get(14) != str(order_qty):
        logger.error(f"Expected CumQty '{order_qty}', got {parsed.get(14)}")
        return False

    # Verify required fields exist
    required_fields = [8, 9, 35, 34, 49, 52, 56, 37, 39, 151, 14, 60, 10]
    for field in required_fields:
        if field not in parsed:
            logger.error(f"Missing required field {field}")
            return False

    # Log execution details
    logger.info(f"✓ ExecutionReport valid:")
    logger.info(f"  - OrderID: {parsed.get(37)}")
    logger.info(f"  - OrdStatus: Filled (2)")
    logger.info(f"  - CumQty: {parsed.get(14)}")
    logger.info(f"  - LeavesQty: {parsed.get(151)}")
    logger.info(f"  - ExecID: {parsed.get(17, 'N/A')}")

    return True


def run_test():
    """Run complete end-to-end test"""
    logger.info("=" * 70)
    logger.info("FIX 4.2 Client Test - Starting")
    logger.info("=" * 70)

    client = FIXClient()

    # Step 1: Connect
    logger.info("\n[STEP 1] Connecting to server...")
    if not client.connect():
        logger.error("Failed to connect")
        return False
    time.sleep(0.5)

    # Step 2: Send Logon
    logger.info("\n[STEP 2] Sending Logon message (35=A)...")
    if not client.send_logon():
        logger.error("Failed to send logon")
        client.disconnect()
        return False

    # Step 3: Receive Logon Response
    logger.info("\n[STEP 3] Waiting for Logon acknowledgment...")
    logon_response = client.receive()
    if not logon_response:
        logger.error("No logon response received")
        client.disconnect()
        return False

    if not validate_logon_response(logon_response):
        logger.error("Logon validation failed")
        client.disconnect()
        return False

    time.sleep(0.5)

    # Step 4: Send NewOrderSingle
    logger.info("\n[STEP 4] Sending NewOrderSingle message (35=D)...")
    if not client.send_new_order_single(symbol='AAPL', qty=100, price=245.50):
        logger.error("Failed to send NewOrderSingle")
        client.disconnect()
        return False

    # Step 5: Receive ExecutionReport
    logger.info("\n[STEP 5] Waiting for ExecutionReport...")
    exec_report = client.receive()
    if not exec_report:
        logger.error("No execution report received")
        client.disconnect()
        return False

    if not validate_execution_report(exec_report, order_qty=100, order_price=245.50):
        logger.error("ExecutionReport validation failed")
        client.disconnect()
        return False

    # Step 6: Disconnect
    logger.info("\n[STEP 6] Disconnecting...")
    client.disconnect()

    logger.info("\n" + "=" * 70)
    logger.info("✓ END-TO-END TEST PASSED")
    logger.info("=" * 70)
    logger.info(f"\nTest log saved to: /tmp/fix_client.log")

    return True


if __name__ == '__main__':
    success = run_test()
    exit(0 if success else 1)
