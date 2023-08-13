CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE SCHEMA IF NOT EXISTS real_medium;

CREATE TABLE IF NOT EXISTS real_medium.users (
    user_id TEXT PRIMARY KEY DEFAULT uuid_generate_v4(),
	username VARCHAR(255) NOT NULL,
	email VARCHAR(255) NOT NULL,
	bio TEXT,
	image VARCHAR(255),
	password_hash VARCHAR(255) NOT NULL,
	CONSTRAINT uniq_username UNIQUE(username),
	CONSTRAINT uniq_email UNIQUE(email)
);

CREATE TABLE IF NOT EXISTS real_medium.articles (
	article_id TEXT PRIMARY KEY DEFAULT uuid_generate_v4(),
	title VARCHAR(255) NOT NULL,
	slug VARCHAR(255) NOT NULL,
	body TEXT NOT NULL,
	description TEXT NOT NULL,
	created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
	updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
        favorites_count BIGINT DEFAULT 0,
	user_id TEXT NOT NULL,
	CONSTRAINT fk_article_author FOREIGN KEY(user_id) REFERENCES real_medium.users(user_id),
	CONSTRAINT uniq_slug UNIQUE(slug)
);

CREATE TABLE IF NOT EXISTS real_medium.tag_list (
	tag_id TEXT PRIMARY KEY DEFAULT uuid_generate_v4(),
	tag_name VARCHAR(255),
	CONSTRAINT uniq_name UNIQUE(tag_name)
);

CREATE TABLE IF NOT EXISTS real_medium.article_tag (
	article_id TEXT NOT NULL,
	tag_id TEXT NOT NULL,
	CONSTRAINT pk_article_tags PRIMARY KEY(article_id, tag_id),
	CONSTRAINT fk_article FOREIGN KEY(article_id) REFERENCES real_medium.articles(article_id) ON DELETE CASCADE,
	CONSTRAINT fk_tag FOREIGN KEY(tag_id) REFERENCES real_medium.tag_list(tag_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS real_medium.favorites (
	article_id TEXT NOT NULL,
	user_id TEXT NOT NULL,
	CONSTRAINT pk_favorites PRIMARY KEY(user_id, article_id),
	CONSTRAINT fk_user FOREIGN KEY(user_id) REFERENCES real_medium.users(user_id),
	CONSTRAINT fk_article FOREIGN KEY(article_id) REFERENCES real_medium.articles(article_id)
);

CREATE TABLE IF NOT EXISTS real_medium.followers (
	followed_user_id TEXT NOT NULL,
	follower_user_id TEXT NOT NULL,
	CONSTRAINT pk_followers PRIMARY KEY(follower_user_id, followed_user_id),
	CONSTRAINT fk_follower FOREIGN KEY(follower_user_id) REFERENCES real_medium.users(user_id),
	CONSTRAINT fk_followed FOREIGN KEY(followed_user_id) REFERENCES real_medium.users(user_id)
);

CREATE TABLE IF NOT EXISTS real_medium.comments (
	comment_id BIGSERIAL,
	created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
	updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
	body VARCHAR(16384) NOT NULL,
	user_id TEXT NOT NULL,
	article_id TEXT NOT NULL,
	CONSTRAINT pk_comments PRIMARY KEY(comment_id),
	CONSTRAINT fk_article FOREIGN KEY(article_id) REFERENCES real_medium.articles(article_id) ON DELETE CASCADE,
	CONSTRAINT fk_author FOREIGN KEY(user_id) REFERENCES real_medium.users(user_id) ON DELETE CASCADE
);




CREATE TYPE real_medium.profile AS
(
        username VARCHAR(255),
        bio TEXT,
        image VARCHAR(255),
        following BOOL
);

CREATE TYPE real_medium.tagged_article_with_author_profile AS
(
        article_id TEXT,
        title VARCHAR(255),
        slug VARCHAR(255),
        body TEXT,
        description TEXT,
        created_at TIMESTAMP WITH TIME ZONE,
        updated_at TIMESTAMP WITH TIME ZONE,
        tagList VARCHAR(255)[],
        favorited BOOL,
        favorites_count BIGINT,
        author real_medium.profile
);




CREATE OR REPLACE FUNCTION real_medium.get_article_tags(
        _article_id TEXT)
    RETURNS SETOF VARCHAR(255)
AS $$
BEGIN
RETURN QUERY
        SELECT
                t.tag_name
        FROM
                real_medium.article_tag AS at
        INNER JOIN
                real_medium.tag_list AS t ON t.tag_id = at.tag_id
        WHERE
                article_id = _article_id
        ORDER BY
                t.tag_name ASC;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.is_following(
        _followed_user_id TEXT,
        _follower_user_id TEXT)
    RETURNS BOOL
AS $$
BEGIN
        RETURN EXISTS (
                SELECT
                        1
                FROM
                        real_medium.followers
                WHERE follower_user_id = _follower_user_id AND followed_user_id = _followed_user_id
        );
END;
$$ LANGUAGE plpgsql;



CREATE OR REPLACE FUNCTION real_medium.create_article(
        _title VARCHAR(255),
        _slug VARCHAR(255),
        _body TEXT,
        _description TEXT,
        _user_id TEXT,
        _tags VARCHAR(255)[])
    RETURNS TEXT
AS $$
DECLARE
        _created_article_id TEXT;
BEGIN
        INSERT INTO
                real_medium.articles (title, slug, body, description, user_id)
        VALUES
                (_title, _slug, _body, _description, _user_id)
        RETURNING
                article_id INTO _created_article_id;

        INSERT INTO
                real_medium.tag_list(tag_name)
        SELECT
                unnest(_tags)
        ON CONFLICT DO NOTHING;

        INSERT INTO
                real_medium.article_tag (article_id, tag_id)
        SELECT
                _created_article_id, tag_ids.tag_id
        FROM (
                SELECT
                        tag_id
                FROM
                        real_medium.tag_list
                WHERE
                        tag_name = ANY (_tags)) AS tag_ids;

        RETURN _created_article_id;
END;
$$ LANGUAGE plpgsql;



CREATE OR REPLACE FUNCTION real_medium.is_favorited_article(
        _article_id TEXT,
        _user_id TEXT = NULL)
    RETURNS BOOL
AS $$
BEGIN
        RETURN EXISTS (	SELECT 1 FROM real_medium.favorites
                WHERE user_id = _user_id AND article_id = _article_id
        );
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_profile(
        _id TEXT,
        _follower_id TEXT = NULL)
    RETURNS SETOF real_medium.profile
AS $$
BEGIN
RETURN QUERY
        SELECT
                username,
                bio,
                image,
                CASE WHEN _follower_id = NULL THEN
                        FALSE
                ELSE
                         real_medium.is_following(_id, _follower_id)
                END
        FROM
                real_medium.users
        WHERE
                user_id = _id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_article_with_author_profile(
        _article_id TEXT,
        _follower_id TEXT = NULL)
    RETURNS SETOF real_medium.tagged_article_with_author_profile
AS $$
BEGIN
        RETURN QUERY
        SELECT
                article_id,
                title,
                slug,
                body,
                description,
                created_at,
                updated_at,
                ARRAY(SELECT * FROM real_medium.get_article_tags(_article_id))::VARCHAR(255)[],
                real_medium.is_favorited_article(_article_id, _follower_id),
                favorites_count,
                real_medium.get_profile(user_id, _follower_id)
        FROM
                real_medium.articles
        WHERE
                article_id = _article_id;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION real_medium.get_article_with_author_profile_by_slug(
        _slug VARCHAR(255),
        _follower_id TEXT = NULL)
    RETURNS SETOF real_medium.tagged_article_with_author_profile
AS $$
DECLARE
        _article_id TEXT;
BEGIN
        SELECT article_id FROM real_medium.articles WHERE slug = _slug INTO _article_id;

        RETURN QUERY
        SELECT
                article_id,
                title,
                slug,
                body,
                description,
                created_at,
                updated_at,
                ARRAY(SELECT * FROM real_medium.get_article_tags(_article_id))::VARCHAR(255)[],
                real_medium.is_favorited_article(_article_id, _follower_id),
                favorites_count,
                real_medium.get_profile(user_id, _follower_id)
        FROM
                real_medium.articles
        WHERE
                article_id = _article_id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_feed_articles(
	_user_id TEXT,
	_limit INT = 20,
	_offset INT = 0)
    RETURNS SETOF real_medium.tagged_article_with_author_profile 
AS $$
BEGIN
	RETURN QUERY
	SELECT 
		article_id,
		title,
		slug,
                body,
		description,	
		created_at,
		updated_at,
		ARRAY(SELECT * FROM real_medium.get_article_tags(article_id))::VARCHAR(255)[],
		real_medium.is_favorited_article(article_id),
		(SELECT COUNT(*) FROM real_medium.favorites WHERE article_id = article_id),
		real_medium.get_profile(user_id, _user_id)
	FROM 
		real_medium.articles
	WHERE
		user_id IN (
			SELECT followed_user_id FROM real_medium.followers WHERE follower_user_id = _user_id
		)		
	ORDER BY created_at DESC
	LIMIT _limit
	OFFSET _offset;
END;
$$ LANGUAGE plpgsql;
