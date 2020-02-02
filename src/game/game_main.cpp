// Tell the engine that this is loaded
#define FOG_GAME

#include <vector>

namespace Game {

Physics::ShapeID rect_shape;
Physics::Body ground;

u32 PLAYER_LAYER = 3;

struct Bullet : public Logic::Entity {
    Physics::Body body;
    f32 offset = 0.20;
    f32 size = 0.05;
    f32 lifetime = 5.0;
    f32 speed = 2.0;
    f32 life;

    void init(Vec2 from, Vec2 target);

    void update(f32 delta);

    void draw();

    REGISTER_NO_FIELDS(BULLET, Bullet);
};

void Bullet::init(Vec2 from, Vec2 target) {
    Vec2 dir = normalize(target - from);
    life = lifetime;
    body = Physics::create_body(rect_shape, 1.0f);
    body.velocity = dir * speed;
    body.position = from + dir * offset;;
    body.scale = V2(1.0, 1.0) * size;
}

void Bullet::update(f32 delta) {
    Physics::integrate(&body, delta);
    life -= delta;
    if (life < 0) {
        Logic::remove_entity(id);
    }
}

void Bullet::draw() {
    Physics::debug_draw_body(&body);
    Renderer::push_rectangle(PLAYER_LAYER, body.position, body.scale,
                             V4(1.0, 1.0, 0.0, 1.0));
}

struct Robot : public Logic::Entity {
    Input::Player player;
    f32 speed = 6.0;
    f32 gravity = 4.0;
    f32 jump_speed = 2.0;
    f32 acc;
    bool jumping;
    bool dash;
    Physics::Body body;
    Logic::EntityID *other_id;

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
}

void Robot::update(f32 delta) {
    f32 movement = 0;
    movement -= Input::down(Input::Name::LEFT, player);
    movement += Input::down(Input::Name::RIGHT, player);
    movement += Input::value(Input::Name::LEFT_RIGHT, player);
    movement *= ABS(movement);
    movement = CLAMP(-1.0, 1.0, movement);

    Util::tweak("movement", &movement);
    Util::tweak("acc", &acc);
    body.velocity += V2(movement * delta * speed, -gravity * delta);
    body.velocity.x *= pow(0.01, delta);
    Physics::integrate(&body, delta);
    Physics::Overlap overlap = Physics::check_overlap(&ground, &body);
    if (overlap) {
        dash = true;
        Physics::solve(overlap);
        if (Input::pressed(Input::Name::JUMP, player)) {
            body.velocity.y = jump_speed;
            jumping = true;
        }
    } else {
        if (Input::pressed(Input::Name::DIVE, player) && dash) {
            dash = false;
            f32 dir = SIGN(body.velocity.x);
            body.velocity += V2(dir, -jump_speed);
        }
    }

    if (jumping && Input::down(Input::Name::JUMP, player) && body.velocity.y > 0) {
        body.velocity.y += gravity * delta * 0.3;
    } else {
        jumping = false;
    }

    if (Input::pressed(Input::Name::SHOOT, player)) {
        Vec2 target = V2(0, 0);
        if (Logic::valid_entity(*other_id))
            target = Logic::fetch_entity<Robot>(*other_id)->body.position;
        Bullet bullet = {};
        bullet.init(body.position, target);
        Logic::add_entity(bullet);
    }

    auto bullet_check = [this](Logic::Entity *e) {
        Bullet *bullet = (Bullet *) e;
        if (Physics::check_overlap(&bullet->body, &this->body)) {
            Logic::remove_entity(e->id);
            Logic::remove_entity(this->id);
        }
        return false;
    };
    Logic::for_entity_of_type(Logic::EntityType::BULLET, bullet_check);
}

void Robot::draw() {
    Renderer::push_rectangle(PLAYER_LAYER, body.position, body.scale);
    Physics::debug_draw_body(&body);
}

void entity_registration() {
    REGISTER_ENTITY(Robot);
    REGISTER_ENTITY(Bullet);
}

Logic::EntityID player1;
Logic::EntityID player2;

void setup() {
    Renderer::turn_on_camera(0);
    Renderer::get_camera(0)->zoom = 0.75;

    Input::add(K(a), Input::Name::LEFT, Input::Player::P1);
    Input::add(K(d), Input::Name::RIGHT, Input::Player::P1);
    Input::add(K(w), Input::Name::JUMP, Input::Player::P1);
    Input::add(K(s), Input::Name::DIVE, Input::Player::P1);
    Input::add(K(e), Input::Name::SHOOT, Input::Player::P1);

    Input::add(A(LEFTX, Input::Player::P1), Input::Name::LEFT_RIGHT, Input::Player::P2);
    Input::add(B(A, Input::Player::P1), Input::Name::JUMP, Input::Player::P2);
    Input::add(B(B, Input::Player::P1), Input::Name::DIVE, Input::Player::P2);
    Input::add(B(RIGHTSHOULDER, Input::Player::P1), Input::Name::SHOOT, Input::Player::P2);

    Vec2 points[] = {
        V2(0, 0),
        V2(0, 1),
        V2(1, 1),
        V2(1, 0),
    };
    rect_shape = Physics::add_shape(LEN(points), points);
    ground = Physics::create_body(rect_shape, 0.0);
    ground.scale.x = 100;
    ground.position.y = -1.0;

    {
        Robot robot = {};
        robot.init(Input::Player::P1, &player2);
        player1 = Logic::add_entity(robot);
    }
    {
        Robot robot = {};
        robot.init(Input::Player::P2, &player1);
        player2 = Logic::add_entity(robot);
    }
}

// Main logic
void update(f32 delta) {
}

// Main draw
void draw() {
    Physics::debug_draw_body(&ground);
    Renderer::push_rectangle(0, ground.position, ground.scale);
}

}  // namespace Game
