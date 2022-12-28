INSERT INTO auth_schema.tokens(token, user_id, scopes, name) VALUES
('wrong-scopes-token', 1, '{"just_wrong"}', 'Bad Man'),
('THE_USER_TOKEN', 2, '{"read", "hello", "info"}', 'Dear User');
