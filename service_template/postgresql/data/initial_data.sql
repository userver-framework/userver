# postgresql template on
INSERT INTO hello_schema.users(name, count)
VALUES ('user-from-initial_data.sql', 42)
ON CONFLICT (name)
DO NOTHING;
# postgresql template off
