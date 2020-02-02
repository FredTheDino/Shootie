// Tell the engine that this is loaded
#define FOG_GAME

#include <vector>

namespace Game {

const Vec4 BULLET_COLOR = V4(0.8, 0.3, 0.8, 1.0);
const Vec4 PLAYER_COLORS[] = {
    V4(0.3, 0.3, 0.8, 1.0),
    V4(0.8, 0.3, 0.3, 1.0),
};

Renderer::ParticleSystem bullet_particles;
Renderer::ParticleSystem land_particles;
Renderer::ParticleSystem hit_particles;

Physics::ShapeID rect_shape;
Physics::ShapeID triangle_shape;
Physics::Body grounds[2];

u32 PLAYER_LAYER = 3;

struct Bullet : public Logic::Entity {
    Physics::Body body;
    f32 offset = 0.20;
    f32 size = 0.10;
    f32 lifetime = 5.0;
    f32 speed = 4.0;
    f32 life;

    void init(Vec2 from, Vec2 target);
    void destroy();

    void update(f32 delta);

    void draw();

    REGISTER_NO_FIELDS(BULLET, Bullet);
};

void Bullet::init(Vec2 from, Vec2 target) {
    Mixer::play_sound(0, ASSET_SHOOT);
    Vec2 dir = normalize(target - from);
    life = lifetime;
    body = Physics::create_body(rect_shape, 1.0f);
    body.velocity = dir * speed;
    body.position = from + dir * offset;;
    body.scale = V2(1.0, 1.0) * size;
}

void Bullet::destroy() {
    Mixer::play_sound(0, ASSET_HIT);
    Logic::remove_entity(id);

    hit_particles.position = body.position;
    for (u32 i = 0; i < 20; i++) {
        hit_particles.spawn();
    }
}

void Bullet::update(f32 delta) {
    Physics::integrate(&body, delta);
    life -= delta;
    bullet_particles.position = body.position;
    bullet_particles.spawn();
    if (life < 0) {
        destroy();
        return;
    }
    for (u32 i = 0; i < LEN(grounds); i++) {
        if (Physics::check_overlap(&body, &grounds[i])) {
            destroy();
            return;
        }
    }
    auto bullet_check = [this](Logic::Entity *e) {
        Bullet *bullet = (Bullet *) e;
        if (e == this) return false;
        if (Physics::check_overlap(&bullet->body, &this->body)) {
            bullet->destroy();
            this->destroy();
            return true;
        }
        return false;
    };
    Logic::for_entity_of_type(Logic::EntityType::BULLET, bullet_check);
}

void Bullet::draw() {
    Renderer::push_rectangle(PLAYER_LAYER, body.position, body.scale, BULLET_COLOR);
}

struct Robot : public Logic::Entity {
    Input::Player player;
    f32 speed = 8.0;
    f32 gravity = 6.0;
    f32 jump_speed = 2.4;
    f32 dash_vel_up = 0.4;
    f32 dash_vel = 6.6;
    f32 acc;
    bool jumping;
    bool dash;
    bool grounded_last_frame;
    Physics::Body body;
    Logic::EntityID *other_id;
    f32 time_to_reload = 0.6;
    f32 reload_timer;
    u32 max_magasin = 3;
    u32 magazin;

    void init(Input::Player p, Logic::EntityID *other_id);

    void update(f32 delta);

    void draw();

    REGISTER_NO_FIELDS(ROBOT, Robot);
};

void Robot::init(Input::Player player, Logic::EntityID *other_id) {
    this->other_id = other_id;
    this->player = player;
    acc = 0;
    body = Physics::create_body(rect_shape, 1.0);
    body.bounce = 0.0;
    body.scale = V2(1, 1) * 0.2;
    magazin = 0;
    reload_timer = Logic::now() + time_to_reload;
}

void Robot::update(f32 delta) {
    if (reload_timer && reload_timer < Logic::now()) {
        magazin = max_magasin;
        reload_timer = 0;
    }
    f32 movement = 0;
    movement -= Input::down(Input::Name::LEFT, player);
    movement += Input::down(Input::Name::RIGHT, player);
    movement += Input::value(Input::Name::LEFT_RIGHT, player);
    movement *= ABS(movement);
    movement = CLAMP(-1.0, 1.0, movement);

    body.velocity += V2(movement * delta * speed, -gravity * delta);
    body.velocity.x *= pow(0.01, delta);
    Physics::integrate(&body, delta);

    body.position.x = CLAMP(-4, 4, body.position.x);

    bool grounded = false;
    for (u32 i = 0; i < LEN(grounds); i++) {
        Physics::Overlap overlap = Physics::check_overlap(&grounds[i], &body);
        if (overlap) {
            dash = true;
            Physics::solve(overlap);
            grounded |= overlap.normal.y > 0.2;
            if (Input::pressed(Input::Name::JUMP, player) && overlap.normal.y > 0.2) {
                Mixer::play_sound(0, ASSET_JUMP);
                body.velocity.y = jump_speed;
                jumping = true;
            }
        }
    }
    if (grounded && !grounded_last_frame) {
        land_particles.position = body.position + V2(0, -0.11);
        for (u32 i = 0; i < 8; i++) {
            land_particles.spawn();
        }
        Mixer::play_sound(0, ASSET_GROUND_HIT);
    }
    grounded_last_frame = grounded;
    if (Input::pressed(Input::Name::DIVE, player) && dash && !grounded) {
        dash = false;
        f32 dir = SIGN(movement + 0.1);
        body.velocity = V2(dir * dash_vel, dash_vel_up);
        Mixer::play_sound(0, ASSET_JUMP, 1.2);
    }

    if (jumping && Input::down(Input::Name::JUMP, player) && body.velocity.y > 0) {
        body.velocity.y += gravity * delta * 0.3;
    } else {
        jumping = false;
    }

    if (Input::pressed(Input::Name::SHOOT, player) && magazin) {
        Vec2 target = V2(0, 0);
        if (Logic::valid_entity(*other_id))
            target = Logic::fetch_entity<Robot>(*other_id)->body.position;
        Bullet bullet = {};
        bullet.init(body.position, target);
        Logic::add_entity(bullet);

        magazin--;
        if (magazin < 1) {
            reload_timer = Logic::now() + time_to_reload;
        }
    }

    auto bullet_check = [this](Logic::Entity *e) {
        Bullet *bullet = (Bullet *) e;
        if (Physics::check_overlap(&bullet->body, &this->body)) {
            bullet->destroy();
            Logic::remove_entity(this->id);
        }
        return false;
    };
    Logic::for_entity_of_type(Logic::EntityType::BULLET, bullet_check);
}

void Robot::draw() {
    Renderer::push_rectangle(PLAYER_LAYER, body.position, body.scale,
                             PLAYER_COLORS[((u32) player) >> 1]);
}

void entity_registration() {
    REGISTER_ENTITY(Robot);
    REGISTER_ENTITY(Bullet);
}

Logic::EntityID player1;
Logic::EntityID player2;

Vec2 spawn_points[] = {
    V2(-2, -0.4),
    V2(-1.3, -0.4),
    V2( 1.3, -0.4),
    V2( 2, -0.4),
};

void spawn_player(Vec2 away_from, Input::Player player) {
    const f32 MIN_DIST = 1.0;
    while (true) {
        Vec2 at = spawn_points[random_int() % LEN(spawn_points)];
        if (length(away_from - at) > MIN_DIST) {
            Robot robot = {};
            if (Input::Player::P1 == player) {
                robot.init(Input::Player::P1, &player2);
                robot.body.position = at;
                player1 = Logic::add_entity(robot);
            } else {
                robot.init(Input::Player::P2, &player1);
                robot.body.position = at;
                player2 = Logic::add_entity(robot);
            }
            return;
        }
    }
}

void setup() {
    Renderer::set_window_title("Do or Die");
    Renderer::set_window_size(1400, 800);
    Renderer::set_fullscreen(false);
    Renderer::turn_on_camera(0);
    Renderer::get_camera(0)->zoom = 0.75;

    bullet_particles = Renderer::create_particle_system(1, 500, V2(0, 0));
    bullet_particles.one_color = true;
    bullet_particles.alive_time = {0.8, 1.3};
    bullet_particles.spawn_size = {0.05, 0.03};
    bullet_particles.velocity = {0, 0};
    bullet_particles.acceleration = {0.1, 0.2};
    bullet_particles.spawn_red = {BULLET_COLOR.x, BULLET_COLOR.x};
    bullet_particles.spawn_green = {BULLET_COLOR.y, BULLET_COLOR.y};
    bullet_particles.spawn_blue = {BULLET_COLOR.z, BULLET_COLOR.z};
    bullet_particles.spawn_alpha = {1, 1};
    bullet_particles.die_alpha = {0, 0};

    hit_particles = Renderer::create_particle_system(1, 500, V2(0, 0));
    hit_particles.one_color = true;
    hit_particles.alive_time = {0.6, 1.1};
    hit_particles.spawn_size = {0.05, 0.03};
    hit_particles.velocity_dir = {0, 2 * PI};
    hit_particles.velocity = {0.4, 0.6};
    hit_particles.acceleration = {0.1, 0.2};
    hit_particles.spawn_red = {BULLET_COLOR.x, BULLET_COLOR.x};
    hit_particles.spawn_green = {BULLET_COLOR.y, BULLET_COLOR.y};
    hit_particles.spawn_blue = {BULLET_COLOR.z, BULLET_COLOR.z};
    hit_particles.spawn_alpha = {1, 1};
    hit_particles.die_alpha = {0, 0};

    land_particles = Renderer::create_particle_system(7, 100, V2(0, 0));
    land_particles.one_color = true;
    land_particles.alive_time = {0.2, 0.4};
    land_particles.spawn_size = {0.00, 0.00};
    land_particles.die_size = {0.01, 0.04};
    land_particles.velocity = {0.2, 0.3};
    land_particles.velocity_dir = {-PI, 0};
    land_particles.spawn_red = {0.5, 0.5};
    land_particles.spawn_green = {0.5, 0.5};
    land_particles.spawn_blue = {0.5, 0.5};
    land_particles.spawn_alpha = {1.0, 1.0};
    land_particles.die_alpha = {0.0, 0.0};

    using namespace Input;
    add(K(a), Name::LEFT,  Player::P1);
    add(K(d), Name::RIGHT, Player::P1);
    add(K(w), Name::JUMP,  Player::P1);
    add(K(s), Name::DIVE,  Player::P1);
    add(K(e), Name::SHOOT, Player::P1);

    add(A(LEFTX, Player::P1), Name::LEFT_RIGHT, Player::P1);
    add(B(DPAD_LEFT, Player::P1), Name::LEFT, Player::P1);
    add(B(DPAD_RIGHT, Player::P1), Name::RIGHT, Player::P1);
    add(B(A, Player::P1), Name::JUMP, Player::P1);
    add(B(B, Player::P1), Name::DIVE, Player::P1);
    add(B(X, Player::P1), Name::DIVE, Player::P1);
    add(B(Y, Player::P1), Name::DIVE, Player::P1);
    add(B(RIGHTSHOULDER, Player::P1), Name::SHOOT, Player::P1);

    add(A(LEFTX, Player::P2), Name::LEFT_RIGHT, Player::P2);
    add(B(DPAD_LEFT, Player::P2), Name::LEFT, Player::P2);
    add(B(DPAD_RIGHT, Player::P2), Name::RIGHT, Player::P2);
    add(B(A, Player::P2), Name::JUMP, Player::P2);
    add(B(B, Player::P2), Name::DIVE, Player::P2);
    add(B(B, Player::P2), Name::DIVE, Player::P2);
    add(B(X, Player::P2), Name::DIVE, Player::P2);
    add(B(Y, Player::P2), Name::DIVE, Player::P2);
    add(B(RIGHTSHOULDER, Player::P2), Name::SHOOT, Player::P2);

    {
        Vec2 points[] = {
            V2(0, 0),
            V2(0, 1),
            V2(1, 1),
            V2(1, 0),
        };
        rect_shape = Physics::add_shape(LEN(points), points);
        Physics::Body ground = Physics::create_body(rect_shape, 0.0);
        ground.scale.x = 100;
        ground.position.y = -1.0;
        grounds[0] = ground;
    }
    {
        Vec2 points[] = {
            V2(-1, 0),
            V2(0, 1),
            V2(1, 0),
        };

        grounds[1] = Physics::create_body(rect_shape, 0.0);
        grounds[1].scale.x = 2;
        grounds[1].scale.y = 0.2;
        grounds[1].position.x = 0.0;
        grounds[1].position.y = -0.5;
    }

    spawn_player(V2(0, 0), Input::Player::P1);
    Vec2 other_pos = Logic::fetch_entity<Robot>(player1)->body.position;
    spawn_player(other_pos, Input::Player::P2);
}

// Main logic
void update(f32 delta) {
    Vec2 positions[3] = {};

    if (!Logic::valid_entity(player1) || !Logic::valid_entity(player2)) {
        auto remove_bullet = [](Logic::Entity *e) {
            Logic::remove_entity(e->id);
            return false;
        };
        Logic::for_entity_of_type(Logic::EntityType::BULLET, remove_bullet);
    }

    if (Logic::valid_entity(player1))
        positions[0] = Logic::fetch_entity<Robot>(player1)->body.position;
    if (Logic::valid_entity(player2))
        positions[1] = Logic::fetch_entity<Robot>(player2)->body.position;

    if (!Logic::valid_entity(player1)) {
        spawn_player(positions[1], Input::Player::P1);
        positions[0] = Logic::fetch_entity<Robot>(player1)->body.position;
    }

    if (!Logic::valid_entity(player2)) {
        spawn_player(positions[0], Input::Player::P2);
        positions[1] = Logic::fetch_entity<Robot>(player2)->body.position;
    }

    static f32 offset = 0.75;
    positions[2] = (positions[0] + positions[1]) / 2 + V2(0, offset);

    Renderer::Camera to = Renderer::camera_fit(LEN(positions), positions, 1.0);
    *Renderer::get_camera(0) = camera_lerp(*Renderer::get_camera(0), to, delta * 2);
    bullet_particles.update(delta);
    land_particles.update(delta);
    hit_particles.update(delta);
}

// Main draw
void draw() {
    bullet_particles.draw();
    land_particles.draw();
    hit_particles.draw();
    Renderer::push_rectangle(0, grounds[0].position, grounds[0].scale);
    Renderer::push_rectangle(0, grounds[1].position, grounds[1].scale);
}

}  // namespace Game
