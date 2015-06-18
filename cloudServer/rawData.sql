INSERT INTO raw (the_geom, latitude, longitude, ax, ay, az, temp, pres, humidity, luminosity, combiner, time) VALUES
(ST_SetSRID(ST_GeomFromGeoJSON('{"type":"Point","coordinates":[29.42599050,40.848944]}'),4326),408489443330,294259905000,     +7,     +2,   +249, +271, 97655, 58.09, 0.18, 87, '9:20:17'),
(ST_SetSRID(ST_GeomFromGeoJSON('{"type":"Point","coordinates":[29.42599050,40.848944]}'),4326),408489443330,294259905000,     -1,     -1,     -1, +270, 97646, 57.48, 0.27, 87, '9:20:17')
;
