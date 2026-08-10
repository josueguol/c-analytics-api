CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email TEXT NOT NULL,
    display_name TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    confirmed_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT users_email_lowercase CHECK (email = lower(email)),
    CONSTRAINT users_email_unique UNIQUE (email),
    CONSTRAINT users_display_name_length CHECK (char_length(display_name) BETWEEN 1 AND 100)
);

CREATE TABLE confirmation_tokens (
    token UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expires_at TIMESTAMPTZ NOT NULL,
    consumed_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX confirmation_tokens_user_idx ON confirmation_tokens(user_id);

CREATE TABLE auth_sessions (
    token UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expires_at TIMESTAMPTZ NOT NULL,
    revoked_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_seen_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX auth_sessions_valid_idx ON auth_sessions(token) WHERE revoked_at IS NULL;

CREATE TABLE favorite_sections (
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    section_key TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (user_id, section_key),
    CONSTRAINT favorite_sections_key_length CHECK (char_length(section_key) BETWEEN 1 AND 160)
);

CREATE TABLE favorite_tags (
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    tag_key TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (user_id, tag_key),
    CONSTRAINT favorite_tags_key_length CHECK (char_length(tag_key) BETWEEN 1 AND 160)
);

-- anonymous_id is a client-generated opaque installation/session identifier;
-- it is intentionally not an email, IP address or other direct identifier.
CREATE TABLE activity_events (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    anonymous_id TEXT,
    portal_key TEXT NOT NULL,
    event_type TEXT NOT NULL,
    content_id TEXT,
    component_id TEXT,
    page_url TEXT,
    event_data JSONB NOT NULL DEFAULT '{}'::jsonb,
    occurred_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    received_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT activity_actor_check CHECK (user_id IS NOT NULL OR anonymous_id IS NOT NULL),
    CONSTRAINT activity_portal_length CHECK (char_length(portal_key) BETWEEN 1 AND 120),
    CONSTRAINT activity_event_type CHECK (event_type IN ('page_view', 'content_view', 'component_click', 'like', 'comment')),
    CONSTRAINT activity_anonymous_id_length CHECK (anonymous_id IS NULL OR char_length(anonymous_id) BETWEEN 8 AND 200)
);
CREATE INDEX activity_events_content_idx ON activity_events (portal_key, content_id, occurred_at DESC)
    WHERE content_id IS NOT NULL;
CREATE INDEX activity_events_component_idx ON activity_events (portal_key, component_id, occurred_at DESC)
    WHERE component_id IS NOT NULL;
CREATE INDEX activity_events_user_idx ON activity_events (user_id, occurred_at DESC)
    WHERE user_id IS NOT NULL;

CREATE TABLE content_likes (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    content_id TEXT NOT NULL,
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    anonymous_id TEXT,
    actor_key TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT content_likes_actor_check CHECK (user_id IS NOT NULL OR anonymous_id IS NOT NULL),
    CONSTRAINT content_likes_actor_unique UNIQUE (content_id, actor_key)
);
CREATE INDEX content_likes_content_idx ON content_likes(content_id);

CREATE TABLE comments (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    content_id TEXT NOT NULL,
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    anonymous_id TEXT,
    author_name TEXT,
    body TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT comments_actor_check CHECK (user_id IS NOT NULL OR anonymous_id IS NOT NULL),
    CONSTRAINT comments_body_length CHECK (char_length(body) BETWEEN 1 AND 5000)
);
CREATE INDEX comments_content_idx ON comments(content_id, created_at DESC);
