INSERT INTO raw (the_geom, latitude, longitude, ax, ay, az, temp, pres, humidity, luminosity, combiner, time) VALUES
(ST_SetSRID(ST_GeomFromGeoJSON('{"type":"Point","coordinates":[4.82020602,39.382119]}'),4326),4820206018,39382118796,     +1,     -1,     +6, +11, 703, 4.91, 4.23, 3, '15:0:11'),
(ST_SetSRID(ST_GeomFromGeoJSON('{"type":"Point","coordinates":[4.82020602,39.382119]}'),4326),4820206018,39382118796,     -1,     +5,     -1, +17, 365, 0.48, 3.73, 3, '15:0:11')
;
