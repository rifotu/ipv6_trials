
-- Database schema example for storing gpsdproxy data.

CREATE TABLE objects (
    id             SERIAL PRIMARY KEY,
    name           CHARACTER VARYING NOT NULL,
    password       CHARACTER VARYING
);


CREATE TABLE positions (
    id             SERIAL PRIMARY KEY,
    idobject       integer NOT NULL,
    inet_addr      character varying,
    rcvd_date_time timestamp without time zone NOT NULL,
    date_time      timestamp without time zone NOT NULL,
    elevation      real,
    speed          real,
    heading        real,
    sat_used       integer,
    hdop           real
);

SELECT AddGeometryColumn('positions', 'wpt', 4326, 'POINT', 2);

COMMENT ON COLUMN positions.idobject IS 'Identifier of transmitting client';
COMMENT ON COLUMN positions.inet_addr IS 'IP address of transmitting client';
COMMENT ON COLUMN positions.rcvd_date_time IS 'UTC timestamp of received data';
COMMENT ON COLUMN positions.date_time IS 'UTC timestamp of GPS position';
COMMENT ON COLUMN positions.elevation IS 'Altitude in meters of GPS position';
COMMENT ON COLUMN positions.speed IS 'Speed in meters/seconds';
COMMENT ON COLUMN positions.heading IS 'Track heading, in degrees. North = 0';
COMMENT ON COLUMN positions.sat_used IS 'Number of satellites used in position calculation';
COMMENT ON COLUMN positions.hdop IS 'Horizontal dilution of precision: 1=good, 20=poor';
COMMENT ON COLUMN positions.wpt IS 'Lat/lon (WGS84) of GPS position';

ALTER TABLE positions ADD UNIQUE (idobject, date_time);
CREATE INDEX idx_positions_date_time ON positions (date_time);
CREATE INDEX idx_positions_idobject ON positions (idobject);
