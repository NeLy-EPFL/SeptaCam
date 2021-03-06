/*
 *       AP_AHRS_MPU6000.cpp
 *
 *       AHRS system using MPU6000's internal calculations
 *
 *       Adapted for the general ArduPilot AHRS interface by Andrew Tridgell
 *
 *       This library is free software; you can redistribute it and/or
 *       modify it under the terms of the GNU Lesser General Public License
 *       as published by the Free Software Foundation; either version 2.1
 *       of the License, or (at your option) any later version.
 */
#include <FastSerial.h>
#include <AP_AHRS.h>

// this is the speed in cm/s above which we first get a yaw lock with
// the GPS
#define GPS_SPEED_MIN 300

// this is the speed in cm/s at which we stop using drift correction
// from the GPS and wait for the ground speed to get above GPS_SPEED_MIN
#define GPS_SPEED_RESET 100

// the limit (in degrees/second) beyond which we stop integrating
// omega_I. At larger spin rates the DCM PI controller can get 'dizzy'
// which results in false gyro drift. See
// http://gentlenav.googlecode.com/files/fastRotations.pdf
#define SPIN_RATE_LIMIT 20

void
AP_AHRS_MPU6000::init()
{
    _mpu6000->dmp_init();
    push_gains_to_dmp();
    push_offsets_to_ins();
};

// run a full MPU6000 update round
void
AP_AHRS_MPU6000::update(void)
{
    float delta_t;

    // tell the IMU to grab some data.  is this necessary?
    _imu->update();

    // ask the IMU how much time this sensor reading represents
    delta_t = _imu->get_delta_time();

    // convert the quaternions into a DCM matrix
    _mpu6000->quaternion.rotation_matrix(_dcm_matrix);

    // we run the gyro bias correction using gravity vector algorithm
    drift_correction(delta_t);

    // Calculate pitch, roll, yaw for stabilization and navigation
    euler_angles();
}

// wrap_PI - ensure an angle (expressed in radians) is between -PI and PI
// TO-DO: should remove and replace with more standard functions
float AP_AHRS_MPU6000::wrap_PI(float angle_in_radians)
{
    if( angle_in_radians > M_PI ) {
        return(angle_in_radians - 2*M_PI);
    }
    else if( angle_in_radians < -M_PI ) {
        return(angle_in_radians + 2*M_PI);
    }
    else{
        return(angle_in_radians);
    }
}

// Function to correct the gyroX and gyroY bias (roll and pitch) using the gravity vector from accelerometers
// We use the internal chip axis definition to make the bias correction because both sensors outputs (gyros and accels)
// are in chip axis definition
void AP_AHRS_MPU6000::drift_correction( float deltat )
{
    float errorRollPitch[2];

    // Get current values for gyros
    _accel_vector = _imu->get_accel();

    // We take the accelerometer readings and cumulate to average them and obtain the gravity vector
    _accel_filtered += _accel_vector;
    _accel_filtered_samples++;

    _gyro_bias_from_gravity_counter++;
    // We make the bias calculation and correction at a lower rate (GYRO_BIAS_FROM_GRAVITY_RATE)
    if( _gyro_bias_from_gravity_counter == GYRO_BIAS_FROM_GRAVITY_RATE ) {
        _gyro_bias_from_gravity_counter = 0;

        _accel_filtered /= _accel_filtered_samples;          // average

        // Adjust ground reference : Accel Cross Gravity to obtain the error between gravity from accels and gravity from attitude solution
        // errorRollPitch are in Accel LSB units
        errorRollPitch[0] = _accel_filtered.y * _dcm_matrix.c.z + _accel_filtered.z * _dcm_matrix.c.x;
        errorRollPitch[1] = -_accel_filtered.z * _dcm_matrix.c.y - _accel_filtered.x * _dcm_matrix.c.z;

        errorRollPitch[0] *= deltat * 1000;
        errorRollPitch[1] *= deltat * 1000;

        // we limit to maximum gyro drift rate on each axis
        float drift_limit = _mpu6000->get_gyro_drift_rate() * deltat / _gyro_bias_from_gravity_gain;          //0.65*0.04 / 0.005 = 5.2
        errorRollPitch[0] = constrain(errorRollPitch[0], -drift_limit, drift_limit);
        errorRollPitch[1] = constrain(errorRollPitch[1], -drift_limit, drift_limit);

        // We correct gyroX and gyroY bias using the error vector
        _gyro_bias[0] += errorRollPitch[0]*_gyro_bias_from_gravity_gain;
        _gyro_bias[1] += errorRollPitch[1]*_gyro_bias_from_gravity_gain;

        // TO-DO: fix this.  Currently it makes the roll and pitch drift more!
        // If bias values are greater than 1 LSB we update the hardware offset registers
        if( fabs(_gyro_bias[0])>1.0 ) {
            //_mpu6000->set_gyro_offsets(-1*(int)_gyro_bias[0],0,0);
            //_mpu6000->set_gyro_offsets(0,-1*(int)_gyro_bias[0],0);
            //_gyro_bias[0] -= (int)_gyro_bias[0];  // we remove the part that we have already corrected on registers...
        }
        if (fabs(_gyro_bias[1])>1.0) {
            //_mpu6000->set_gyro_offsets(-1*(int)_gyro_bias[1],0,0);
            //_gyro_bias[1] -= (int)_gyro_bias[1];
        }

        // Reset the accelerometer variables
        _accel_filtered.x = 0;
        _accel_filtered.y = 0;
        _accel_filtered.z = 0;
        _accel_filtered_samples=0;
    }

    // correct the yaw
    drift_correction_yaw();
}


/*
 *  reset the DCM matrix and omega. Used on ground start, and on
 *  extreme errors in the matrix
 */
void
AP_AHRS_MPU6000::reset(bool recover_eulers)
{
    // if the caller wants us to try to recover to the current
    // attitude then calculate the dcm matrix from the current
    // roll/pitch/yaw values
    if (recover_eulers && !isnan(roll) && !isnan(pitch) && !isnan(yaw)) {
        _dcm_matrix.from_euler(roll, pitch, yaw);
    } else {
        // otherwise make it flat
        _dcm_matrix.from_euler(0, 0, 0);
    }
}

// push offsets down from IMU to INS (required so MPU6000 can perform it's own attitude estimation)
void
AP_AHRS_MPU6000::push_offsets_to_ins()
{
    // push down gyro offsets (TO-DO: why are x and y offsets are reversed?!)
    _mpu6000->set_gyro_offsets_scaled(((AP_IMU_INS*)_imu)->gy(),((AP_IMU_INS*)_imu)->gx(),((AP_IMU_INS*)_imu)->gz());
    ((AP_IMU_INS*)_imu)->gx(0);
    ((AP_IMU_INS*)_imu)->gy(0);
    ((AP_IMU_INS*)_imu)->gz(0);

    // push down accelerometer offsets (TO-DO: why are x and y offsets are reversed?!)
    _mpu6000->set_accel_offsets_scaled(((AP_IMU_INS*)_imu)->ay(), ((AP_IMU_INS*)_imu)->ax(), ((AP_IMU_INS*)_imu)->az());
    //((AP_IMU_INS*)_imu)->ax(0);
    //((AP_IMU_INS*)_imu)->ay(0);
    //((AP_IMU_INS*)_imu)->az(0);
}

void
AP_AHRS_MPU6000::push_gains_to_dmp()
{
    uint8_t gain;
    if( _kp.get() >= 1.0 ) {
        gain = 0xFF;
    }else if( _kp.get() <= 0.0 ) {
        gain = 0x00;
    }else{
        gain = (uint8_t)((float)0xFF * _kp.get());
    }

    _mpu6000->dmp_set_sensor_fusion_accel_gain(gain);
}

// produce a yaw error value. The returned value is proportional
// to sin() of the current heading error in earth frame
float
AP_AHRS_MPU6000::yaw_error_compass(void)
{
    Vector3f mag = Vector3f(_compass->mag_x, _compass->mag_y, _compass->mag_z);
    // get the mag vector in the earth frame
    Vector3f rb = _dcm_matrix * mag;

    rb.normalize();
    if (rb.is_inf()) {
        // not a valid vector
        return 0.0;
    }

    // get the earths magnetic field (only X and Y components needed)
    Vector3f mag_earth = Vector3f(cos(_compass->get_declination()),
                                  sin(_compass->get_declination()), 0);

    // calculate the error term in earth frame
    Vector3f error = rb % mag_earth;

    return error.z;
}

//
// drift_correction_yaw - yaw drift correction using the compass
//
void
AP_AHRS_MPU6000::drift_correction_yaw(void)
{
    static float yaw_last_uncorrected = HEADING_UNKNOWN;
    static float yaw_corrected = HEADING_UNKNOWN;
    float yaw_delta = 0;
    bool new_value = false;
    float yaw_error;
    static float heading;

    // we assume the DCM matrix is completely uncorrect for yaw
    // retrieve how much heading has changed according to non-corrected dcm
    if( yaw_last_uncorrected != HEADING_UNKNOWN ) {
        yaw_delta = wrap_PI(yaw - yaw_last_uncorrected);                // the change in heading according to the gyros only since the last interation
        yaw_last_uncorrected = yaw;
    }

    // initialise yaw_corrected (if necessary)
    if( yaw_corrected != HEADING_UNKNOWN ) {
        yaw_corrected = yaw;
    }else{
        yaw_corrected = wrap_PI(yaw_corrected + yaw_delta);             // best guess of current yaw is previous iterations corrected yaw + yaw change from gyro
        _dcm_matrix.from_euler(roll, pitch, yaw_corrected);                     // rebuild dcm matrix with best guess at current yaw
    }

    // if we have new compass data
    if( _compass && _compass->use_for_yaw() ) {
        if (_compass->last_update != _compass_last_update) {
            _compass_last_update = _compass->last_update;
            heading = _compass->calculate_heading(_dcm_matrix);
            if( !_have_initial_yaw ) {
                yaw_corrected = heading;
                _have_initial_yaw = true;
                _dcm_matrix.from_euler(roll, pitch, yaw_corrected);                             // rebuild dcm matrix with best guess at current yaw
            }
            new_value = true;
        }
    }

    // perform the yaw correction
    if( new_value ) {
        yaw_error = yaw_error_compass();
        // the yaw error is a vector in earth frame
        //Vector3f error = Vector3f(0,0, yaw_error);

        // convert the error vector to body frame
        //error = _dcm_matrix.mul_transpose(error);

        // Update the differential component to reduce the error (it?s like a P control)
        yaw_corrected += wrap_PI(yaw_error * _kp_yaw.get());                    // probably completely wrong

        // rebuild the dcm matrix yet again
        _dcm_matrix.from_euler(roll, pitch, yaw_corrected);
    }
}

// calculate the euler angles which will be used for high level
// navigation control
void
AP_AHRS_MPU6000::euler_angles(void)
{
    _dcm_matrix.to_euler(&roll, &pitch, &yaw);
    //quaternion.to_euler(&roll, &pitch, &yaw);	// cannot use this because the quaternion is not correct for yaw drift

    roll_sensor     = degrees(roll)  * 100;
    pitch_sensor    = degrees(pitch) * 100;
    yaw_sensor              = degrees(yaw)   * 100;

    if (yaw_sensor < 0)
        yaw_sensor += 36000;
}

/* reporting of DCM state for MAVLink */

// average error_roll_pitch since last call
float AP_AHRS_MPU6000::get_error_rp(void)
{
    // not yet supported with DMP
    return 0.0;
}

// average error_yaw since last call
float AP_AHRS_MPU6000::get_error_yaw(void)
{
    // not yet supported with DMP
    return 0.0;
}
