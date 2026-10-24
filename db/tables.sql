-- ==========================================
--  Temperature Sensor Hypertable
-- ==========================================
CREATE TABLE device_temperature (
    id BIGSERIAL,
    device_name VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45),
    temperature_value INT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id, created_at)
);

SELECT create_hypertable('device_temperature', 'created_at');


-- ==========================================
--  CO2 Sensor Hypertable
-- ==========================================
CREATE TABLE device_co2 (
    id BIGSERIAL,
    device_name VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45),
    co2_value INT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id, created_at)
);

SELECT create_hypertable('device_co2', 'created_at');


-- ==========================================
--  Ultrasonic Sensor Hypertable
-- ==========================================
CREATE TABLE devultrasensor (
    id BIGSERIAL,
    device_name VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45),
    distance_value INT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id, created_at)
);

SELECT create_hypertable('devultrasensor', 'created_at');