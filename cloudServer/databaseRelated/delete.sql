--DELETE FROM raw WHERE (NOW() - INTERVAL '1 day') < created_at
DELETE FROM raw *
