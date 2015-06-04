--SELECT humidity FROM raw WHERE (NOW() - INTERVAL '1 day') < created_at
SELECT humidity FROM raw ORDER BY created_at DESC LIMIT 5;
