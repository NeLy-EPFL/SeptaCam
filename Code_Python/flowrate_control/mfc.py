# -*- coding: utf-8 -*-

"""
Modified from the PyExpLabSys project (https://github.com/CINF/PyExpLabSys).
PyExpLabSys is challenging to install because its submodules have mixed
compatibility with Python 3. This script is core essentials from the package's
vogtlin driver that can run in a stand-alone manner.

This script is also largely based on work by Lauren Wright in her Spring 2021
olfactometer semester project.

========== ORIGINAL DOCSTRING BELOW ==========

Minimal MODBUS driver for the red-y smart - meter GSM, - controller GSC,
- pressure controller GSP and - back pressure controller GSB.

Implemented from the communication manual which is valid for instruments with a serial
number starting from 110 000.

The manual can be downloaded from this page: https://www.voegtlin.com/en/support/download/
and has this link: https://www.voegtlin.com/data/329-3042_en_manualsmart_digicom.pdf-

@author: Kenneth Nielsen <k.nielsen81@gmail.com>
"""

from time import time, sleep

import serial
import minimalmodbus



DEFAULT_COM_KWARGS = {
    'BAUDRATE': 9600,
    'BYTESIZE': 8,
    'STOPBITS': 2,
    'PARITY': serial.PARITY_NONE,
}


class MFC:
    """Control interface for a Red-y MFC.

    Adapted from Lauren Wright's olfactometer semester project
    (spring 2021) and PyExpLabSys (https://github.com/CINF/PyExpLabSys).
    """
    def __init__(self, port, address):
        """
        Parameters
        ----------
        port : str
            The serial port name, for example ``/dev/ttyUSB0``. This can
            be found (on Linux) by simply running ``ls /dev/tty*`` before
            and after plugging in the device and comparing the outputs.
        address : int
            Slave address of the device as found in the Red-y software.
        """
        self.port = port
        self.address = address
        self._instrument = minimalmodbus.Instrument(
            port, address,
            close_port_after_each_call=True
        )
        self._instrument.serial.baudrate = 9600
        self._instrument.serial.stopbits = 2
        self._flow_meter = _RedFlowMeter(port, address)
    
    def set_rate(self, rate, settling_time=0.1):
        """Change the flow rate. Unit can be either mL/min or L/min
        depending on the model.
        
        Parameters
        ----------
        rate : float
            The desired flow rate.
        settling_time : float, default=0.1
            The amount of time (in seconds) to wait before returning.
            This ensures that the flow rate is sufficiently settled before
            the caller moves on.
        
        Raises
        ------
        ValueError
            If the desired flow rate is out of bound.
        """
        try:
            self._flow_meter.write_value('setpoint_gas_flow', rate)
        except minimalmodbus.IllegalRequestError as e:
            raise ValueError('Flow rate out of bound')
        sleep(settling_time)
    
    def get_rate(self):
        """Read the current flow rate. Unit can be either mL/min or L/min
        depending on the model.
        
        Returns
        -------
        float
            Current flow rate.
        """
        return self._flow_meter.read_flow()
    


def _process_string(value):
    """Strip a few non-ascii characters from string"""
    return value.strip('\x00\x16')


def _convert_version(value):
    """Extract 3 version numbers from 2 bytes"""
    # subversion bits 0.3, versin bits 4-7, type bits 8-15
    type_ = value // 256
    value = value % 256
    version = value // 16
    subversion = value % 16
    return '{}.{}.{}'.format(type_, version, subversion)


class _RedFlowMeter(object):
    """Driver for the red-y smart flow meter"""

    # The command map consist of:
    # name: (minimalmodbus_method, conversion_function), method_args...)
    # The command is generate from pages 1.14 and 1.15 from the manual
    command_map = {
        'flow': (('read_float', None), 0x00),
        'temperature': (('read_float', None), 0x02),
        'address': (('read_register', None), 0x0013),
        'serial': (('read_long', None), 0x001e),
        'hardware_version': (('read_register', _convert_version), 0x0020),
        'software_version': (('read_register', _convert_version), 0x0021),
        'type_code_1': (('read_string', _process_string), 0x0023, 4),
        'type_code_2h': (('read_string', _process_string), 0x1004, 4),
        'lut_select': (('read_register', None), 0x4139),
        'range': (('read_float', None), 0x6020),
        'fluid_name': (('read_string', _process_string), 0x6042, 4),
        'unit': (('read_string', _process_string), 0x6046, 4),
        'control_function': (('read_register', None), 0x000e),
    }
    # The command map for set operations consists of
    # name: (minimalmodbus_method, conversion_function, address)
    command_map_set = {
        'setpoint_gas_flow': ('write_float', None, 0x0006),
    }


    def __init__(self, port, slave_address, **serial_com_kwargs):
        """Initialize driver

        Args:
            port (str): Device name e.g. "COM4" or "/dev/serial/by-id/XX-YYYYY
            slave_address (int): The integer slave address
            serial_com_kwargs (dict): Mapping with setting to value for the serial
                communication settings available for minimalmodbus at the module level.
                E.g. to set `minimalmodbus.BAUDRATE` use {'BAUDRATE': 9600}.
        """
        # Apply serial communications settings to minimalmodbus module
        self.serial_com_kwargs = dict(DEFAULT_COM_KWARGS)
        self.serial_com_kwargs.update(serial_com_kwargs)
        for key, value in self.serial_com_kwargs.items():
            setattr(minimalmodbus, key, value)

        # Initialize the instrument
        self.instrument = minimalmodbus.Instrument(port, slave_address)
        self._last_call = time()
        # Specify number of retrys when reading data
        self.number_of_retries = 10

    def _ensure_waittime(self):
        """Ensure waittime"""
        waittime = 0.004 / 9600 * self.serial_com_kwargs['BAUDRATE']
        time_to_sleep = waittime - (time() - self._last_call)
        if time_to_sleep > 0:
            sleep(time_to_sleep)

    def read_value(self, value_name):
        """Read a value

        Args:
            value_name (str): The name of the value to read. Valid values are the keys in
                self.command_map

        Raises:
            ValueError: On invalid key
        """
        # Ensure waittime
        self._ensure_waittime()

        # Extract command_spec
        try:
            command_spec = self.command_map[value_name]
        except KeyError:
            msg = "Invalid value name. Valid names are: {}"
            raise ValueError(msg.format(list(self.command_map.keys())))

        # The command_spec is:
        # name: (minimalmodbus_method, conversion_function), method_args...)
        method_name, conversion_function = command_spec[0]

        for retry_number in range(1, self.number_of_retries):
            try:
                method = getattr(self.instrument, method_name)
                value = method(*command_spec[1:])
                if conversion_function is not None:
                    value = conversion_function(value)
                break
            except IOError as e:
                print("I/O error({}): {}. Trying to retrieve data again..".format(retry_number, e))
                sleep(0.5)
                continue
            except ValueError as e:
                print("ValueError({}): {}. Trying to retrieve data again..".format(retry_number, e))
                sleep(0.5)
                continue
        else:
            raise RuntimeError('Could not retrieve data in\
                                       {} retries'.format(self.number_of_retries))
        # Set last call time
        self._last_call = time()

        return value

    def write_value(self, value_name, value):
        """Write a value

        Args:
            value_name (str): The name of the value to read. Valid values are the keys in
                self.command_map
            value (object): The value to write

        Raises:
            ValueError: On invalid key
        """
        # Ensure waittime
        self._ensure_waittime()

        # Extract command_spec
        try:
            command_spec = self.command_map_set[value_name]
        except KeyError:
            msg = "Invalid value name. Valid names are: {}"
            raise ValueError(msg.format(list(self.command_map_set.keys())))

        # The command_spec for set is:
        # name: (minimalmodbus_method, conversion_function, address)
        method_name, conversion_function, address = command_spec
        method = getattr(self.instrument, method_name)
        if conversion_function:
            value = conversion_function(value)
        method(address, value)

        # Set last call time
        self._last_call = time()

        return value

    def read_all(self):
        """Return all values"""
        return {name: self.read_value(name) for name in self.command_map.keys()}

    def read_flow(self):
        """Return the current flow (alias for read_value('flow')"""
        return self.read_value('flow')

    def read_temperature(self):
        """Return the current temperature"""
        return self.read_value('temperature')

    def set_address(self, address):
        """Set the modbus address

        Args:
            address (int): The slave address to use 1-247

        Raise:
            ValueError: On invalid address
        """
        if not (isinstance(address, int) and address in range(1, 248)):
            msg = 'Invalid address: {}. Must be in range 1-247'
            raise ValueError(msg.format(address))
        self.instrument.address = address