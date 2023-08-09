CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

DROP SCHEMA IF EXISTS real_medium CASCADE;
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
	favorites_count INT,
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
