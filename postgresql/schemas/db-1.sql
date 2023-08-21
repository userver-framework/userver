CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE SCHEMA IF NOT EXISTS real_medium;

CREATE TABLE IF NOT EXISTS real_medium.users(
        user_id text PRIMARY KEY DEFAULT uuid_generate_v4(),
        username varchar(255) NOT NULL,
        email varchar(255) NOT NULL,
        bio text,
        image varchar(255),
        password_hash varchar(255) NOT NULL,
        CONSTRAINT uniq_username UNIQUE (username),
        CONSTRAINT uniq_email UNIQUE (email)
);

CREATE TABLE IF NOT EXISTS real_medium.articles(
        article_id text PRIMARY KEY DEFAULT uuid_generate_v4(),
        title varchar(255) NOT NULL,
        slug varchar(255) NOT NULL,
        body text NOT NULL,
        description text NOT NULL,
        created_at timestamptz NOT NULL DEFAULT NOW(),
        updated_at timestamptz NOT NULL DEFAULT NOW(),
        favorites_count bigint DEFAULT 0,
        user_id text NOT NULL,
        CONSTRAINT fk_article_author FOREIGN KEY (user_id) REFERENCES real_medium.users(user_id),
        CONSTRAINT uniq_slug UNIQUE (slug)
);

CREATE TABLE IF NOT EXISTS real_medium.tag_list(
        tag_id text PRIMARY KEY DEFAULT uuid_generate_v4(),
        tag_name varchar(255),
        CONSTRAINT uniq_name UNIQUE (tag_name)
);

CREATE TABLE IF NOT EXISTS real_medium.article_tag(
        article_id text NOT NULL,
        tag_id text NOT NULL,
        CONSTRAINT pk_article_tags PRIMARY KEY (article_id, tag_id),
        CONSTRAINT fk_article FOREIGN KEY (article_id) REFERENCES real_medium.articles(article_id) ON DELETE CASCADE,
        CONSTRAINT fk_tag FOREIGN KEY (tag_id) REFERENCES real_medium.tag_list(tag_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS real_medium.favorites(
        article_id text NOT NULL,
        user_id text NOT NULL,
        CONSTRAINT pk_favorites PRIMARY KEY (user_id, article_id),
        CONSTRAINT fk_user FOREIGN KEY (user_id) REFERENCES real_medium.users(user_id) ON DELETE CASCADE,
        CONSTRAINT fk_article FOREIGN KEY (article_id) REFERENCES real_medium.articles(article_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS real_medium.followers(
        followed_user_id text NOT NULL,
        follower_user_id text NOT NULL,
        CONSTRAINT pk_followers PRIMARY KEY (follower_user_id, followed_user_id),
        CONSTRAINT fk_follower FOREIGN KEY (follower_user_id) REFERENCES real_medium.users(user_id),
        CONSTRAINT fk_followed FOREIGN KEY (followed_user_id) REFERENCES real_medium.users(user_id)
);

CREATE TABLE IF NOT EXISTS real_medium.comments(
        comment_id bigserial,
        created_at timestamptz NOT NULL DEFAULT NOW(),
        updated_at timestamptz NOT NULL DEFAULT NOW(),
        body varchar(16384) NOT NULL,
        user_id text NOT NULL,
        article_id text NOT NULL,
        CONSTRAINT pk_comments PRIMARY KEY (comment_id),
        CONSTRAINT fk_article FOREIGN KEY (article_id) REFERENCES real_medium.articles(article_id) ON DELETE CASCADE,
        CONSTRAINT fk_author FOREIGN KEY (user_id) REFERENCES real_medium.users(user_id) ON DELETE CASCADE
);



CREATE TYPE real_medium.user AS (
        user_id text,
        username varchar(255),
        email text,
        bio TEXT,
        image VARCHAR(255),  
        password_hash TEXT
);

CREATE TYPE real_medium.full_article_info AS (
        article_id text,
        title varchar(255),
        slug  varchar(255),
        description text,
        body text,
        tags VARCHAR(255)[],
        created_at TIMESTAMP WITH TIME ZONE,
        updated_at TIMESTAMP WITH TIME ZONE,
        article_favorited_by_user_ids  TEXT[],
        article_favorited_by_usernames TEXT[],
        author_followed_by_user_ids    TEXT[],
        author real_medium.user
);

CREATE TYPE real_medium.profile AS (
        username varchar( 255),
        bio TEXT,
        image VARCHAR(255),
        FOLLOWING BOOL);


CREATE TYPE real_medium.tagged_article_with_author_profile AS (
        article_id text,
        title varchar( 255),
        slug VARCHAR(255),
        body TEXT,
        description TEXT,
        created_at TIMESTAMP WITH TIME ZONE,
        updated_at TIMESTAMP WITH TIME ZONE,
        tagList VARCHAR(255)[],
        favorited BOOL,
        favorites_count BIGINT,
        author real_medium.profile);

CREATE OR REPLACE FUNCTION real_medium.get_article_tags(_article_id text)
        RETURNS SETOF VARCHAR (
                255
)
        AS $$
BEGIN
        RETURN QUERY
        SELECT
                t.tag_name
        FROM
                real_medium.article_tag AS at
                INNER JOIN real_medium.tag_list AS t ON t.tag_id = at.tag_id
        WHERE
                article_id = _article_id
        ORDER BY
                t.tag_name ASC;
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.is_following(_followed_user_id text, _follower_user_id text)
        RETURNS bool
        AS $$
BEGIN
        RETURN EXISTS(
                SELECT
                        1
                FROM
                        real_medium.followers
                WHERE
                        follower_user_id = _follower_user_id
                        AND followed_user_id = _followed_user_id);
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.create_article(_title varchar(255), _slug varchar(255), _body text, _description text, _user_id text, _tags varchar(255)[])
        RETURNS text
        AS $$
DECLARE
        _created_article_id text;
BEGIN
        INSERT INTO real_medium.articles(title, slug, body, description, user_id)
                VALUES (_title, _slug, _body, _description, _user_id)
        RETURNING
                article_id INTO _created_article_id;
        INSERT INTO real_medium.tag_list(tag_name)
        SELECT
                unnest(_tags)
        ON CONFLICT
                DO NOTHING;
        INSERT INTO real_medium.article_tag(article_id, tag_id)
        SELECT
                _created_article_id,
                tag_ids.tag_id
        FROM (
                SELECT
                        tag_id
                FROM
                        real_medium.tag_list
                WHERE
                        tag_name = ANY (_tags)) AS tag_ids;
        RETURN _created_article_id;
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.is_favorited_article(_article_id text, _user_id text = NULL)
        RETURNS bool
        AS $$
BEGIN
        RETURN EXISTS(
                SELECT
                        1
                FROM
                        real_medium.favorites
                WHERE
                        user_id = _user_id
                        AND article_id = _article_id);
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_profile(_id text, _follower_id text = NULL)
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
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_article_with_author_profile(_article_id text, _follower_id text = NULL)
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
                ARRAY(
                        SELECT
                                *
                        FROM
                                real_medium.get_article_tags(_article_id))::varchar(255)[],
        real_medium.is_favorited_article(_article_id, _follower_id),
        favorites_count,
        real_medium.get_profile(user_id, _follower_id)
FROM
        real_medium.articles
WHERE
        article_id = _article_id;
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_article_with_author_profile_by_slug(_slug varchar(255), _follower_id text = NULL)
        RETURNS SETOF real_medium.tagged_article_with_author_profile
        AS $$
DECLARE
        _article_id text;
BEGIN
        SELECT
                article_id
        FROM
                real_medium.articles
        WHERE
                slug = _slug INTO _article_id;
        RETURN QUERY
        SELECT
                article_id,
                title,
                slug,
                body,
                description,
                created_at,
                updated_at,
                ARRAY (
                        SELECT
                                *
                        FROM
                                real_medium.get_article_tags(_article_id))::varchar(255)[],
        real_medium.is_favorited_article(_article_id, _follower_id),
        favorites_count,
        real_medium.get_profile(user_id, _follower_id)
FROM
        real_medium.articles
WHERE
        article_id = _article_id;
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_feed_articles(_user_id text, _limit int = 20, _offset int = 0)
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
                ARRAY(
                        SELECT
                                *
                        FROM
                                real_medium.get_article_tags(article_id))::varchar(255)[],
        real_medium.is_favorited_article(article_id, _user_id),
        favorites_count,
        real_medium.get_profile(user_id, _user_id)
FROM
        real_medium.articles
WHERE
        user_id IN(
                SELECT
                        followed_user_id
                FROM
                        real_medium.followers
                WHERE
                        follower_user_id = _user_id)
ORDER BY
        created_at DESC
LIMIT _limit OFFSET _offset;
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.update_article_by_slug(_old_slug varchar(255), _user_id text, _title varchar(255) = NULL, _new_slug varchar(255) = NULL, _description text = NULL, _body text = NULL)
        RETURNS SETOF TEXT
        AS $$
BEGIN
        RETURN QUERY UPDATE
                real_medium.articles
        SET
                title = COALESCE(_title, title),
                slug = COALESCE(_new_slug, slug),
                description = COALESCE(_description, description),
                body = COALESCE(_body, body),
                updated_at = NOW()
        WHERE
                slug = _old_slug
                AND user_id = _user_id
        RETURNING
                article_id;
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_article_id_by_slug(_slug varchar(255))
        RETURNS SETOF TEXT
        AS $$
BEGIN
        RETURN QUERY
        SELECT
                article_id
        FROM
                real_medium.articles
        WHERE
                slug = _slug;
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.delete_article_by_slug(_slug varchar(255), _user_id text)
        RETURNS SETOF TEXT
        AS $$
BEGIN
        RETURN QUERY DELETE FROM real_medium.articles
        WHERE slug = _slug
                AND user_id = _user_id
        RETURNING
                article_id;
END;
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION real_medium.get_articles_by_filters(_user_id text = NULL, _tag text = NULL, _author text = NULL, _favorited text = NULL, _limit int = 20, _offset int = 0)
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
                ARRAY(
                        SELECT
                                *
                        FROM
                                real_medium.get_article_tags(article_id))::varchar(255)[],
        real_medium.is_favorited_article(article_id, _user_id),
        favorites_count,
        real_medium.get_profile(user_id, _user_id)
FROM
        real_medium.articles
WHERE(_tag IS NULL
                OR article_id IN(
                        SELECT
                                article_id
                        FROM
                                real_medium.article_tag
                        WHERE
                                tag_id IN(
                                        SELECT
                                                tag_id
                                        FROM
                                                real_medium.tag_list
                                        WHERE
                                                tag_name = _tag)))
                AND(_author IS NULL
                        OR article_id IN(
                                SELECT
                                        article_id
                                FROM
                                        real_medium.users
                                        INNER JOIN real_medium.articles USING(user_id)
                                WHERE
                                        username = _author))
                AND(_favorited IS NULL
                        OR article_id IN(
                                SELECT
                                        article_id
                                FROM
                                        real_medium.users
                                        INNER JOIN real_medium.favorites USING(user_id)
                                WHERE
                                        username = _favorited))
        ORDER BY
                created_at DESC
        LIMIT _limit OFFSET _offset;
END;
$$
LANGUAGE plpgsql;

