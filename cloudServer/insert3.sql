INSERT INTO map2_copy (the_geom, latitude, longitude, ph) VALUES 
(ST_SetSRID(ST_GeomFromGeoJSON('{"type":"Point","coordinates":[27.66406775,38.58182166]}'),4326),27.66406775,38.58182166, 14),
(ST_SetSRID(ST_GeomFromGeoJSON('{"type":"Point","coordinates":[28.66406775,38.58182166]}'),4326),28.66406775,38.58182166, 15),
(ST_SetSRID(ST_GeomFromGeoJSON('{"type":"Point","coordinates":[29.66406775,38.58182166]}'),4326),29.66406775,38.58182166, 18)
;
