#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <DHT.h>
#include <Preferences.h>
#include <math.h>

const int I2C_SDA_PIN = 6;
const int I2C_SCL_PIN = 7;
const int DHT_PIN = 3;
const int BUTTON_ONE_PIN = 4;
const int BUTTON_TWO_PIN = 5;

const bool USE_DHT11 = true;

const uint8_t OLED_ADDRESS = 0x3C;
const uint8_t MPU6050_ADDRESS = 0x68;

const int STARTING_JOY = 70;
const int STARTING_ENERGY = 75;
const int STARTING_FULLNESS = 65;

const bool RESET_SAVED_PET_ON_BOOT = false;

enum PetReaction {
  NAP_REACTION,
  JUMP_REACTION,
  HEART_REACTION,
  RUN_REACTION,
};

struct MenuItem {
  const char *label;
  int joyChange;
  int energyChange;
  int fullnessChange;
  PetReaction reaction;
};

const MenuItem MENU_ITEMS[] = {
  {"NAP",   1,  18, -4, NAP_REACTION},
  {"PLAY", 12, -9, -5, RUN_REACTION},
  {"FEED",  3,  2,  18, JUMP_REACTION},
  {"PET",   7,  0,  0, HEART_REACTION},
};
const int MENU_ITEM_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

const float MENU_TILT_LIMIT = 6.0f;

const bool SWAP_MPU_AXES = false;
const float MENU_X_DIRECTION = 1.0f;
const float MENU_Y_DIRECTION = -1.0f;

const float MENU_CENTER_DEADZONE = 0.8f;
const float SHAKE_THRESHOLD = 7.0f;

const int SHAKE_JOY_CHANGE = 5;
const int SHAKE_ENERGY_CHANGE = -2;
const int SHAKE_FULLNESS_CHANGE = -1;

const float HOT_TEMP_C = 30.0f;
const float COLD_TEMP_C = 15.0f;
const float HUMID_PERCENT = 80.0f;
const float HOT_SPEED_FACTOR = 0.6f;

const float GRAVITY_DEADZONE = 1.2f;
const float GRAVITY_MAX_TILT = 6.0f;
const float GRAVITY_PULL = 30.0f;
const float GRAVITY_MAX_SPEED = 95.0f;

const uint32_t IDLE_MIN_MS = 5000;
const uint32_t IDLE_MAX_MS = 12000;
const float BUG_SPEED = 14.0f;
const float BUG_CHASE_SPEED = 40.0f;
const int BUG_CATCH_JOY = 2;

const uint16_t GAME_HOLD_MS = 900;
const float GAME_TILT_LIMIT = 5.0f;
const int GAME_START_LIVES = 3;
const int GAME_BAD_PERCENT = 25;
const int GAME_JOY_PER_POINT = 2;
const int GAME_MAX_JOY_REWARD = 30;
const int GAME_ENERGY_COST = 8;
const int GAME_MAX_FULLNESS_REWARD = 10;

const int PET_SPRITE_WIDTH = 32;
const int PET_SPRITE_HEIGHT = 32;
const bool SPRITE_FACES_RIGHT = false;

const uint16_t PET_WALK_PIXEL_MS = 70;
const uint16_t PET_PRE_JUMP_MS = 230;
const uint16_t PET_JUMP_MS = 430;
const int PET_JUMP_HEIGHT = 16;
const uint32_t NAP_DURATION_MS = 48000;
const uint16_t HEARTS_DURATION_MS = 1600;
const uint16_t PLAY_LAP_MS = 800;
const uint8_t PLAY_LAP_COUNT = 2;

const uint8_t PROGMEM PET_SPRITE[] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x70, 0x0a, 0x00,
  0x00, 0xf8, 0x1f, 0x00,
  0x00, 0x8c, 0x3f, 0x80,
  0x00, 0x43, 0xff, 0x00,
  0x00, 0x43, 0xff, 0x00,
  0x00, 0x30, 0x7e, 0x00,
  0x00, 0x98, 0x01, 0x00,
  0x01, 0x98, 0x01, 0x80,
  0x03, 0x00, 0x10, 0xc0,
  0x03, 0x04, 0x00, 0xc0,
  0x01, 0x8b, 0x01, 0x80,
  0x01, 0xcb, 0x03, 0x80,
  0x03, 0xc0, 0x03, 0xc0,
  0x03, 0xc0, 0x03, 0xc0,
  0x01, 0xc0, 0x03, 0x80,
  0x01, 0x80, 0x01, 0x80,
  0x00, 0x60, 0x06, 0x00,
  0x00, 0x3f, 0xfc, 0x00,
  0x00, 0x7f, 0xfe, 0x00,
  0x00, 0x70, 0x0e, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int MENU_CENTER_X = 64;
const int MENU_CENTER_Y = 32;
const int MENU_ITEM_X[] = {64, 108, 64, 20};
const int MENU_ITEM_Y[] = {12, 32, 53, 32};
const int MENU_BALL_X_RANGE = 37;
const int MENU_BALL_Y_RANGE = 22;
const int GAME_MAX_ITEMS = 6;
const uint16_t BUTTON_DEBOUNCE_MS = 30;
const uint16_t MPU_READ_INTERVAL_MS = 30;
const uint16_t DHT_READ_INTERVAL_MS = 2200;
const uint16_t SHAKE_COOLDOWN_MS = 650;
const float STANDARD_GRAVITY = 9.80665f;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;
DHT dht(DHT_PIN, DHT11);
Preferences preferences;

struct PetState {
  int joy;
  int energy;
  int fullness;
};

struct ButtonState {
  int pin;
  bool stableState;
  bool lastRawState;
  uint32_t lastChangedAt;
};

struct FallingItem {
  float x;
  float y;
  float speed;
  bool bad;
  bool active;
};

enum View {
  PET_VIEW,
  MENU_VIEW,
  STATS_VIEW,
  GAME_VIEW,
};

enum ButtonEvent {
  BUTTON_NONE,
  BUTTON_DOWN,
  BUTTON_UP,
};

enum IdleKind {
  IDLE_NONE,
  IDLE_LOOK,
  IDLE_HOP,
  IDLE_SNEEZE,
  IDLE_HUM,
  IDLE_BUG,
};

PetState pet = {STARTING_JOY, STARTING_ENERGY, STARTING_FULLNESS};
ButtonState buttonOne = {BUTTON_ONE_PIN, HIGH, HIGH, 0};
ButtonState buttonTwo = {BUTTON_TWO_PIN, HIGH, HIGH, 0};
View currentView = PET_VIEW;

bool mpuFound = false;
bool dhtFound = false;
float accelerationX = 0.0f;
float accelerationY = 0.0f;
float accelerationZ = STANDARD_GRAVITY;
float restAccelerationX = 0.0f;
float restAccelerationY = 0.0f;
float temperatureC = NAN;
float humidity = NAN;
float menuBallX = MENU_CENTER_X;
float menuBallY = MENU_CENTER_Y;
float menuCenterAccelerationX = 0.0f;
float menuCenterAccelerationY = 0.0f;
int selectedMenuItem = -1;
uint32_t lastMpuReadAt = 0;
uint32_t lastDhtReadAt = 0;
uint32_t lastShakeAt = 0;
uint32_t shakeAnimationEndsAt = 0;
uint32_t petJumpStartedAt = 0;
uint32_t nappingUntil = 0;
uint32_t heartAnimationEndsAt = 0;
uint32_t playRunStartedAt = 0;

uint32_t lastFrameAt = 0;
float frameSeconds = 0.016f;

float petX = 48.0f;
float petVelocity = 0.0f;
int petDirection = 1;
bool petFacingRight = false;

IdleKind idleKind = IDLE_NONE;
uint32_t idleStartedAt = 0;
uint32_t idleEndsAt = 0;
uint32_t nextIdleAt = 0;
float bugX = 0.0f;
int bugDirection = 1;
uint32_t bugTurnAt = 0;
bool bugCaught = false;

uint8_t flippedSprite[(PET_SPRITE_WIDTH / 8) * PET_SPRITE_HEIGHT];

uint32_t buttonTwoDownAt = 0;
bool buttonTwoHoldHandled = true;

FallingItem items[GAME_MAX_ITEMS];
float gamePlayerX = 48.0f;
bool gameFacingRight = true;
float gameCenterAccelerationX = 0.0f;
float gameCenterAccelerationY = 0.0f;
int gameScore = 0;
int gameLives = 0;
int gameBest = 0;
bool gameOver = false;
bool gameRewarded = false;
bool gameNewBest = false;
uint32_t gameNextSpawnAt = 0;
uint32_t gamePopupUntil = 0;
bool gamePopupBad = false;

int keepInRange(int value, int smallest, int largest) {
  return constrain(value, smallest, largest);
}

void changePet(int joyChange, int energyChange, int fullnessChange) {
  pet.joy = keepInRange(pet.joy + joyChange, 0, 100);
  pet.energy = keepInRange(pet.energy + energyChange, 0, 100);
  pet.fullness = keepInRange(pet.fullness + fullnessChange, 0, 100);
}

void loadPet() {
  preferences.begin("starbie", false);
  if (RESET_SAVED_PET_ON_BOOT) {
    preferences.clear();
  }
  pet.joy = preferences.getInt("joy", STARTING_JOY);
  pet.energy = preferences.getInt("energy", STARTING_ENERGY);
  pet.fullness = preferences.getInt("full", STARTING_FULLNESS);
  gameBest = preferences.getInt("best", 0);
}

void savePet() {
  preferences.putInt("joy", pet.joy);
  preferences.putInt("energy", pet.energy);
  preferences.putInt("full", pet.fullness);
}

void setUpButton(ButtonState &button) {
  pinMode(button.pin, INPUT_PULLUP);
  button.stableState = digitalRead(button.pin);
  button.lastRawState = button.stableState;
  button.lastChangedAt = millis();
}

ButtonEvent readButton(ButtonState &button) {
  const bool rawState = digitalRead(button.pin);
  const uint32_t now = millis();

  if (rawState != button.lastRawState) {
    button.lastRawState = rawState;
    button.lastChangedAt = now;
  }

  if (rawState != button.stableState &&
      now - button.lastChangedAt >= BUTTON_DEBOUNCE_MS) {
    button.stableState = rawState;
    return button.stableState == LOW ? BUTTON_DOWN : BUTTON_UP;
  }

  return BUTTON_NONE;
}

void updateMpu() {
  if (!mpuFound || millis() - lastMpuReadAt < MPU_READ_INTERVAL_MS) {
    return;
  }
  lastMpuReadAt = millis();

  sensors_event_t acceleration;
  sensors_event_t gyro;
  sensors_event_t sensorTemperature;
  mpu.getEvent(&acceleration, &gyro, &sensorTemperature);
  accelerationX = acceleration.acceleration.x;
  accelerationY = acceleration.acceleration.y;
  accelerationZ = acceleration.acceleration.z;
}

void calibrateRestPose() {
  const int samples = 10;
  float sumX = 0.0f;
  float sumY = 0.0f;
  for (int i = 0; i < samples; i++) {
    sensors_event_t acceleration;
    sensors_event_t gyro;
    sensors_event_t sensorTemperature;
    mpu.getEvent(&acceleration, &gyro, &sensorTemperature);
    sumX += acceleration.acceleration.x;
    sumY += acceleration.acceleration.y;
    delay(8);
  }
  restAccelerationX = sumX / samples;
  restAccelerationY = sumY / samples;
  accelerationX = restAccelerationX;
  accelerationY = restAccelerationY;
}

void updateDht() {
  if (!dhtFound || millis() - lastDhtReadAt < DHT_READ_INTERVAL_MS) {
    return;
  }
  lastDhtReadAt = millis();

  const float newHumidity = dht.readHumidity();
  const float newTemperatureC = dht.readTemperature();
  if (!isnan(newHumidity)) {
    humidity = newHumidity;
  }
  if (!isnan(newTemperatureC)) {
    temperatureC = newTemperatureC;
  }
}

bool isHot() {
  return !isnan(temperatureC) && temperatureC >= HOT_TEMP_C;
}

bool isCold() {
  return !isnan(temperatureC) && temperatureC <= COLD_TEMP_C;
}

bool isHumid() {
  return !isnan(humidity) && humidity >= HUMID_PERCENT;
}

float horizontalTilt() {
  const float raw = SWAP_MPU_AXES ? accelerationY - restAccelerationY
                                  : accelerationX - restAccelerationX;
  return constrain(raw * MENU_X_DIRECTION, -GRAVITY_MAX_TILT, GRAVITY_MAX_TILT);
}

uint32_t playRunDuration() {
  return static_cast<uint32_t>(PLAY_LAP_MS) * PLAY_LAP_COUNT;
}

bool isNapping() {
  return nappingUntil != 0 && millis() < nappingUntil;
}

bool isPlaying() {
  return playRunStartedAt != 0 && millis() - playRunStartedAt < playRunDuration();
}

int playRunX(uint32_t now) {
  const int farthestX = SCREEN_WIDTH - PET_SPRITE_WIDTH;
  const uint32_t lapAge = (now - playRunStartedAt) % PLAY_LAP_MS;
  const float progress = static_cast<float>(lapAge) / PLAY_LAP_MS;
  return progress < 0.5f ? static_cast<int>(progress * 2.0f * farthestX)
                         : static_cast<int>((1.0f - progress) * 2.0f * farthestX);
}

bool playFacingRight(uint32_t now) {
  const uint32_t lapAge = (now - playRunStartedAt) % PLAY_LAP_MS;
  return lapAge < PLAY_LAP_MS / 2;
}

void updatePetTimers() {
  if (nappingUntil != 0 && !isNapping()) {
    nappingUntil = 0;
  }
  if (playRunStartedAt != 0 && !isPlaying()) {
    playRunStartedAt = 0;
  }
}

void scheduleNextIdle(uint32_t now) {
  nextIdleAt = now + random(static_cast<long>(IDLE_MIN_MS), static_cast<long>(IDLE_MAX_MS));
}

void cancelIdle(uint32_t now) {
  idleKind = IDLE_NONE;
  bugCaught = false;
  scheduleNextIdle(now);
}

bool idleStandsStill() {
  if (idleKind == IDLE_BUG) {
    return bugCaught;
  }
  return idleKind != IDLE_NONE;
}

void startIdle(uint32_t now) {
  int choice = random(0, 5);
  if (isCold() && random(0, 3) == 0) {
    choice = 2;
  }
  idleStartedAt = now;
  bugCaught = false;

  switch (choice) {
    case 0:
      idleKind = IDLE_LOOK;
      idleEndsAt = now + 1800;
      break;
    case 1:
      idleKind = IDLE_HOP;
      idleEndsAt = now + 800;
      break;
    case 2:
      idleKind = IDLE_SNEEZE;
      idleEndsAt = now + 1500;
      break;
    case 3:
      idleKind = IDLE_HUM;
      idleEndsAt = now + 2400;
      break;
    default: {
      idleKind = IDLE_BUG;
      idleEndsAt = now + 9000;
      const float center = petX + PET_SPRITE_WIDTH / 2.0f;
      bugX = center < 64.0f ? random(80, 115) : random(14, 48);
      bugDirection = random(0, 2) == 0 ? -1 : 1;
      bugTurnAt = now + 500;
      break;
    }
  }
}

void updateBug(uint32_t now) {
  if (bugCaught) {
    return;
  }

  if (now >= bugTurnAt) {
    bugDirection = random(0, 2) == 0 ? -1 : 1;
    bugTurnAt = now + random(400, 1000);
  }
  bugX += bugDirection * BUG_SPEED * frameSeconds;
  if (bugX < 14.0f) {
    bugX = 14.0f;
    bugDirection = 1;
  }
  if (bugX > 114.0f) {
    bugX = 114.0f;
    bugDirection = -1;
  }

  if (petJumpStartedAt == 0 &&
      fabsf(petX + PET_SPRITE_WIDTH / 2.0f - bugX) < 9.0f) {
    bugCaught = true;
    petJumpStartedAt = now;
    heartAnimationEndsAt = now + HEARTS_DURATION_MS;
    idleEndsAt = now + PET_PRE_JUMP_MS + PET_JUMP_MS + 300;
    changePet(BUG_CATCH_JOY, 0, 0);
    savePet();
  }
}

void updateIdle(uint32_t now) {
  if (idleKind == IDLE_NONE) {
    const bool free = currentView == PET_VIEW && !isNapping() && !isPlaying() &&
                      petJumpStartedAt == 0 && now >= shakeAnimationEndsAt &&
                      now >= heartAnimationEndsAt &&
                      fabsf(horizontalTilt()) <= GRAVITY_DEADZONE;
    if (free && now >= nextIdleAt) {
      startIdle(now);
    }
    return;
  }

  if (currentView != PET_VIEW || isNapping() || isPlaying()) {
    cancelIdle(now);
    return;
  }

  if (idleKind == IDLE_BUG) {
    updateBug(now);
  }

  if (now >= idleEndsAt && petJumpStartedAt == 0) {
    cancelIdle(now);
  }
}

float walkSpeed() {
  float speed = 1000.0f / PET_WALK_PIXEL_MS;
  if (isHot()) {
    speed *= HOT_SPEED_FACTOR;
  }
  return speed;
}

void updatePetMotion(uint32_t now) {
  if (currentView != PET_VIEW) {
    return;
  }

  const float farthestX = SCREEN_WIDTH - PET_SPRITE_WIDTH;

  if (isPlaying()) {
    petX = playRunX(now);
    petVelocity = 0.0f;
    petFacingRight = playFacingRight(now);
    return;
  }
  if (isNapping()) {
    petVelocity = 0.0f;
    return;
  }

  const float tilt = horizontalTilt();
  const bool tilted = mpuFound && fabsf(tilt) > GRAVITY_DEADZONE;

  if (tilted) {
    if (idleKind != IDLE_NONE) {
      cancelIdle(now);
    }
    petVelocity += tilt * GRAVITY_PULL * frameSeconds;
    petVelocity -= petVelocity * fminf(1.0f, 0.6f * frameSeconds);
    petVelocity = constrain(petVelocity, -GRAVITY_MAX_SPEED, GRAVITY_MAX_SPEED);
    if (fabsf(petVelocity) > 3.0f) {
      petFacingRight = petVelocity > 0.0f;
      petDirection = petFacingRight ? 1 : -1;
    }
  } else if (idleStandsStill()) {
    petVelocity -= petVelocity * fminf(1.0f, 8.0f * frameSeconds);
  } else if (idleKind == IDLE_BUG) {
    const float difference = bugX - PET_SPRITE_WIDTH / 2.0f - petX;
    const int direction = difference > 0.0f ? 1 : -1;
    petFacingRight = direction > 0;
    petDirection = direction;
    petVelocity += (direction * BUG_CHASE_SPEED - petVelocity) *
                   fminf(1.0f, 6.0f * frameSeconds);
  } else {
    petFacingRight = petDirection > 0;
    petVelocity += (petDirection * walkSpeed() - petVelocity) *
                   fminf(1.0f, 3.0f * frameSeconds);
  }

  petX += petVelocity * frameSeconds;
  if (petX <= 0.0f) {
    petX = 0.0f;
    petVelocity = 0.0f;
    petDirection = 1;
  } else if (petX >= farthestX) {
    petX = farthestX;
    petVelocity = 0.0f;
    petDirection = -1;
  }
}

void updateMenuBall() {
  float xTilt = accelerationX - menuCenterAccelerationX;
  float yTilt = accelerationY - menuCenterAccelerationY;

  if (SWAP_MPU_AXES) {
    const float oldXTilt = xTilt;
    xTilt = yTilt;
    yTilt = oldXTilt;
  }
  xTilt = constrain(xTilt * MENU_X_DIRECTION, -MENU_TILT_LIMIT, MENU_TILT_LIMIT);
  yTilt = constrain(yTilt * MENU_Y_DIRECTION, -MENU_TILT_LIMIT, MENU_TILT_LIMIT);
  const float targetX = MENU_CENTER_X + (xTilt / MENU_TILT_LIMIT) * MENU_BALL_X_RANGE;
  const float targetY = MENU_CENTER_Y + (yTilt / MENU_TILT_LIMIT) * MENU_BALL_Y_RANGE;

  menuBallX += (targetX - menuBallX) * 0.20f;
  menuBallY += (targetY - menuBallY) * 0.20f;

  if (fabsf(xTilt) < MENU_CENTER_DEADZONE && fabsf(yTilt) < MENU_CENTER_DEADZONE) {
    selectedMenuItem = -1;
  } else if (fabsf(xTilt) > fabsf(yTilt)) {
    selectedMenuItem = xTilt > 0.0f ? 1 : 3;
  } else {
    selectedMenuItem = yTilt > 0.0f ? 2 : 0;
  }
}

void checkForShake() {
  if (!mpuFound || currentView != PET_VIEW) {
    return;
  }

  const float magnitude = sqrtf(accelerationX * accelerationX +
                                accelerationY * accelerationY +
                                accelerationZ * accelerationZ);
  const uint32_t now = millis();
  if (fabsf(magnitude - STANDARD_GRAVITY) >= SHAKE_THRESHOLD &&
      now - lastShakeAt >= SHAKE_COOLDOWN_MS) {
    lastShakeAt = now;
    nappingUntil = 0;
    shakeAnimationEndsAt = now + 350;
    cancelIdle(now);
    changePet(SHAKE_JOY_CHANGE, SHAKE_ENERGY_CHANGE, SHAKE_FULLNESS_CHANGE);
    savePet();
  }
}

void openMenu() {
  currentView = MENU_VIEW;
  selectedMenuItem = -1;
  menuBallX = MENU_CENTER_X;
  menuBallY = MENU_CENTER_Y;
  menuCenterAccelerationX = accelerationX;
  menuCenterAccelerationY = accelerationY;
}

void chooseMenuItem() {
  if (selectedMenuItem == -1) {
    return;
  }

  const MenuItem &item = MENU_ITEMS[selectedMenuItem];
  changePet(item.joyChange, item.energyChange, item.fullnessChange);
  savePet();
  const uint32_t now = millis();
  nappingUntil = 0;
  heartAnimationEndsAt = 0;
  playRunStartedAt = 0;
  cancelIdle(now);

  if (item.reaction == NAP_REACTION) {
    petJumpStartedAt = 0;
    petVelocity = 0.0f;
    nappingUntil = now + NAP_DURATION_MS;
  } else if (item.reaction == RUN_REACTION) {
    petJumpStartedAt = 0;
    playRunStartedAt = now;
    heartAnimationEndsAt = now + playRunDuration();
  } else {
    petJumpStartedAt = now;
    if (item.reaction == HEART_REACTION) {
      heartAnimationEndsAt = now + HEARTS_DURATION_MS;
    }
  }
  currentView = PET_VIEW;
}

void startGame() {
  const uint32_t now = millis();
  cancelIdle(now);
  nappingUntil = 0;
  playRunStartedAt = 0;
  petJumpStartedAt = 0;
  heartAnimationEndsAt = 0;

  currentView = GAME_VIEW;
  gameScore = 0;
  gameLives = GAME_START_LIVES;
  gameOver = false;
  gameRewarded = false;
  gameNewBest = false;
  gamePopupUntil = 0;
  gamePlayerX = (SCREEN_WIDTH - PET_SPRITE_WIDTH) / 2.0f;
  gameFacingRight = true;
  gameCenterAccelerationX = accelerationX;
  gameCenterAccelerationY = accelerationY;
  gameNextSpawnAt = now + 800;
  for (int i = 0; i < GAME_MAX_ITEMS; i++) {
    items[i].active = false;
  }
}

void rewardGame() {
  if (gameRewarded) {
    return;
  }
  gameRewarded = true;

  if (gameScore > gameBest) {
    gameBest = gameScore;
    gameNewBest = true;
    preferences.putInt("best", gameBest);
  }
  if (gameScore > 0) {
    int joyReward = gameScore * GAME_JOY_PER_POINT;
    if (joyReward > GAME_MAX_JOY_REWARD) {
      joyReward = GAME_MAX_JOY_REWARD;
    }
    int fullnessReward = gameScore;
    if (fullnessReward > GAME_MAX_FULLNESS_REWARD) {
      fullnessReward = GAME_MAX_FULLNESS_REWARD;
    }
    changePet(joyReward, -GAME_ENERGY_COST, fullnessReward);
    savePet();
  }
}

void leaveGame() {
  const uint32_t now = millis();
  rewardGame();
  currentView = PET_VIEW;
  if (gameScore > 0) {
    petJumpStartedAt = now;
    heartAnimationEndsAt = now + HEARTS_DURATION_MS;
  }
  scheduleNextIdle(now);
}

void loseLife(uint32_t now, bool wasBad) {
  gameLives--;
  gamePopupUntil = now + 450;
  gamePopupBad = true;
  if (gameLives <= 0) {
    gameLives = 0;
    gameOver = true;
    rewardGame();
  }
  (void)wasBad;
}

void spawnItem(uint32_t now) {
  for (int i = 0; i < GAME_MAX_ITEMS; i++) {
    if (!items[i].active) {
      items[i].active = true;
      items[i].x = random(8, SCREEN_WIDTH - 8);
      items[i].y = -4.0f;
      items[i].bad = gameScore >= 3 && random(0, 100) < GAME_BAD_PERCENT;
      items[i].speed = 24.0f + fminf(gameScore * 1.2f, 36.0f) + random(0, 10);
      break;
    }
  }
  int gap = 1000 - gameScore * 18;
  if (gap < 420) {
    gap = 420;
  }
  gameNextSpawnAt = now + gap + random(0, 250);
}

void updateGame(uint32_t now) {
  if (gameOver) {
    return;
  }

  float tilt = SWAP_MPU_AXES ? accelerationY - gameCenterAccelerationY
                             : accelerationX - gameCenterAccelerationX;
  tilt = constrain(tilt * MENU_X_DIRECTION / GAME_TILT_LIMIT, -1.0f, 1.0f);
  const float halfTrack = (SCREEN_WIDTH - PET_SPRITE_WIDTH) / 2.0f;
  const float target = halfTrack + tilt * halfTrack;
  const float previousX = gamePlayerX;
  gamePlayerX += (target - gamePlayerX) * 0.35f;
  if (fabsf(gamePlayerX - previousX) > 0.4f) {
    gameFacingRight = gamePlayerX > previousX;
  }

  if (now >= gameNextSpawnAt) {
    spawnItem(now);
  }

  const float catchTop = SCREEN_HEIGHT - PET_SPRITE_HEIGHT + 6.0f;
  for (int i = 0; i < GAME_MAX_ITEMS && !gameOver; i++) {
    if (!items[i].active) {
      continue;
    }
    items[i].y += items[i].speed * frameSeconds;

    const bool inCatchRange = items[i].x >= gamePlayerX + 4.0f &&
                              items[i].x <= gamePlayerX + PET_SPRITE_WIDTH - 4.0f;
    if (items[i].y + 3.0f >= catchTop && inCatchRange) {
      items[i].active = false;
      if (items[i].bad) {
        loseLife(now, true);
      } else {
        gameScore++;
        gamePopupUntil = now + 450;
        gamePopupBad = false;
      }
    } else if (items[i].y > SCREEN_HEIGHT + 4) {
      items[i].active = false;
      if (!items[i].bad) {
        loseLife(now, false);
      }
    }
  }
}

void handleButtons() {
  const uint32_t now = millis();

  if (readButton(buttonOne) == BUTTON_DOWN) {
    if (currentView == MENU_VIEW) {
      chooseMenuItem();
    } else if (currentView == GAME_VIEW) {
      if (gameOver) {
        startGame();
      }
    } else {
      openMenu();
    }
  }

  const ButtonEvent buttonTwoEvent = readButton(buttonTwo);
  if (buttonTwoEvent == BUTTON_DOWN) {
    buttonTwoDownAt = now;
    buttonTwoHoldHandled = false;
  }

  if (buttonTwo.stableState == LOW && !buttonTwoHoldHandled &&
      currentView == PET_VIEW && now - buttonTwoDownAt >= GAME_HOLD_MS) {
    buttonTwoHoldHandled = true;
    startGame();
  }

  if (buttonTwoEvent == BUTTON_UP && !buttonTwoHoldHandled) {
    if (currentView == GAME_VIEW) {
      leaveGame();
    } else {
      currentView = currentView == STATS_VIEW ? PET_VIEW : STATS_VIEW;
    }
  }
}

void buildFlippedSprite() {
  const int bytesPerRow = PET_SPRITE_WIDTH / 8;
  memset(flippedSprite, 0, sizeof(flippedSprite));
  for (int row = 0; row < PET_SPRITE_HEIGHT; row++) {
    for (int col = 0; col < PET_SPRITE_WIDTH; col++) {
      const uint8_t sourceByte = pgm_read_byte(&PET_SPRITE[row * bytesPerRow + col / 8]);
      if (sourceByte & (0x80 >> (col % 8))) {
        const int flippedCol = PET_SPRITE_WIDTH - 1 - col;
        flippedSprite[row * bytesPerRow + flippedCol / 8] |= (0x80 >> (flippedCol % 8));
      }
    }
  }
}

void drawSprite(int x, int y, bool faceRight) {
  if (faceRight != SPRITE_FACES_RIGHT) {
    display.drawBitmap(x, y, flippedSprite, PET_SPRITE_WIDTH, PET_SPRITE_HEIGHT,
                       SSD1306_WHITE);
  } else {
    display.drawBitmap(x, y, PET_SPRITE, PET_SPRITE_WIDTH, PET_SPRITE_HEIGHT,
                       SSD1306_WHITE);
  }
}

void drawHeart(int x, int y) {
  display.fillRect(x - 2, y, 2, 2, SSD1306_WHITE);
  display.fillRect(x + 1, y, 2, 2, SSD1306_WHITE);
  display.fillRect(x - 3, y + 2, 7, 2, SSD1306_WHITE);
  display.fillRect(x - 2, y + 4, 5, 1, SSD1306_WHITE);
  display.fillRect(x - 1, y + 5, 3, 1, SSD1306_WHITE);
  display.drawPixel(x, y + 6, SSD1306_WHITE);
}

void drawHearts(uint32_t now, int petDrawX, int petDrawY) {
  if (now >= heartAnimationEndsAt) {
    return;
  }

  const float progress =
      1.0f - static_cast<float>(heartAnimationEndsAt - now) / HEARTS_DURATION_MS;
  const int rise = static_cast<int>(progress * 18.0f);
  drawHeart(petDrawX + 10, petDrawY - 3 - rise);
  drawHeart(petDrawX + 22, petDrawY - 9 - rise / 2);
}

void drawSleepZs(uint32_t now, int petDrawX, int petDrawY) {
  const int rise = static_cast<int>((now / 300UL) % 15UL);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(petDrawX + 23, petDrawY - 2 - rise);
  display.print("z");
  display.setCursor(petDrawX + 28, petDrawY - 8 - rise / 2);
  display.print("z");
}

void drawRain(uint32_t now) {
  for (int i = 0; i < 10; i++) {
    const int x = (i * 37 + 11) % SCREEN_WIDTH;
    const int y = static_cast<int>((now / 12UL + i * 29UL) % 72UL) - 8;
    display.drawFastVLine(x, y, 3, SSD1306_WHITE);
  }
}

void drawWeather(uint32_t now, int petDrawX, int petDrawY) {
  if (!isHot()) {
    return;
  }
  for (int i = 0; i < 2; i++) {
    const int fall = static_cast<int>((now / 130UL + i * 5UL) % 11UL);
    const int dropX = i == 0 ? petDrawX + 4 : petDrawX + 27;
    display.fillRect(dropX, petDrawY + 8 + fall, 2, 3, SSD1306_WHITE);
  }
}

void drawBug(uint32_t now) {
  const int x = static_cast<int>(bugX);
  const int y = SCREEN_HEIGHT - 6 - static_cast<int>((now / 90UL) % 2UL);
  display.fillRect(x - 1, y, 3, 2, SSD1306_WHITE);
  if ((now / 70UL) % 2UL == 0) {
    display.drawPixel(x - 2, y - 1, SSD1306_WHITE);
    display.drawPixel(x + 2, y - 1, SSD1306_WHITE);
  } else {
    display.drawPixel(x - 2, y + 1, SSD1306_WHITE);
    display.drawPixel(x + 2, y + 1, SSD1306_WHITE);
  }
}

void applyIdlePose(uint32_t now, int &x, int &y, bool &faceRight) {
  const uint32_t age = now - idleStartedAt;

  if (idleKind == IDLE_LOOK) {
    if ((age / 450UL) % 2UL == 1UL) {
      faceRight = !faceRight;
    }
  } else if (idleKind == IDLE_HOP) {
    if (age < 700) {
      y -= static_cast<int>(fabsf(sinf(age * PI / 350.0f)) * 8.0f);
    }
  } else if (idleKind == IDLE_SNEEZE) {
    const int back = faceRight ? -1 : 1;
    if (age >= 300 && age < 650) {
      x += back * 2;
    } else if (age >= 650 && age < 950) {
      x += ((now / 30UL) % 2UL == 0UL) ? 3 : -3;
    }
  } else if (idleKind == IDLE_HUM) {
    x += static_cast<int>(sinf(age / 180.0f) * 2.0f);
  }
}

void drawIdleOverlay(uint32_t now, int petDrawX, int petDrawY, bool faceRight) {
  const uint32_t age = now - idleStartedAt;

  if (idleKind == IDLE_SNEEZE && age >= 650) {
    if (age < 1100) {
      const int direction = faceRight ? 1 : -1;
      const int frontX = faceRight ? petDrawX + 27 : petDrawX + 4;
      const int travel = static_cast<int>((age - 650) / 40);
      for (int i = 0; i < 3; i++) {
        display.drawPixel(frontX + direction * (travel + i * 3), petDrawY + 14 + i,
                          SSD1306_WHITE);
      }
    }
    const int textX = constrain(petDrawX - 3, 0, SCREEN_WIDTH - 36);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(textX, petDrawY > 10 ? petDrawY - 10 : 0);
    display.print("ACHOO!");
  } else if (idleKind == IDLE_HUM) {
    const int riseOne = static_cast<int>((age / 200UL) % 10UL);
    const int riseTwo = static_cast<int>((age / 200UL + 5UL) % 10UL);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(petDrawX + 6, petDrawY - 4 - riseOne);
    display.write(0x0E);
    display.setCursor(petDrawX + 20, petDrawY - 10 - riseTwo);
    display.write(0x0E);
  } else if (idleKind == IDLE_BUG && !bugCaught) {
    drawBug(now);
  }
}

void drawPet() {
  display.clearDisplay();

  const uint32_t now = millis();
  if (isHumid()) {
    drawRain(now);
  }

  int x = static_cast<int>(petX);
  int y = SCREEN_HEIGHT - PET_SPRITE_HEIGHT;
  bool faceRight = petFacingRight;

  if (now < shakeAnimationEndsAt) {
    x += static_cast<int>(sinf(now / 18.0f) * 3.0f);
  }

  if (!isNapping() && petJumpStartedAt != 0) {
    const uint32_t animationAge = now - petJumpStartedAt;

    if (animationAge < PET_PRE_JUMP_MS) {
      x += static_cast<int>(sinf(now / 16.0f) * 3.0f);
    } else if (animationAge < PET_PRE_JUMP_MS + PET_JUMP_MS) {
      const float jumpProgress =
          static_cast<float>(animationAge - PET_PRE_JUMP_MS) / PET_JUMP_MS;
      y -= static_cast<int>(sinf(jumpProgress * PI) * PET_JUMP_HEIGHT);
    } else {
      petJumpStartedAt = 0;
    }
  }

  if (idleKind != IDLE_NONE) {
    applyIdlePose(now, x, y, faceRight);
  }

  if (isCold()) {
    x += ((now / 50UL) % 2UL == 0UL) ? 1 : -1;
  }

  if (!isNapping() && petJumpStartedAt == 0 && fabsf(petVelocity) > 6.0f) {
    y -= static_cast<int>((now / 140UL) % 2UL);
  }

  x = constrain(x, 0, SCREEN_WIDTH - PET_SPRITE_WIDTH);
  drawSprite(x, y, faceRight);

  if (isNapping()) {
    drawSleepZs(now, x, y);
  }
  drawHearts(now, x, y);
  drawWeather(now, x, y);
  drawIdleOverlay(now, x, y, faceRight);
}

void drawMenuItem(int item) {
  const int boxWidth = 33;
  const int boxHeight = 12;
  const int boxX = MENU_ITEM_X[item] - boxWidth / 2;
  const int boxY = MENU_ITEM_Y[item] - boxHeight / 2;
  const bool isSelected = item == selectedMenuItem;

  if (isSelected) {
    display.fillRoundRect(boxX, boxY, boxWidth, boxHeight, 3, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRoundRect(boxX, boxY, boxWidth, boxHeight, 3, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }

  display.setTextSize(1);
  const int labelLength = strlen(MENU_ITEMS[item].label);
  display.setCursor(MENU_ITEM_X[item] - labelLength * 3, MENU_ITEM_Y[item] - 3);
  display.print(MENU_ITEMS[item].label);
  display.setTextColor(SSD1306_WHITE);
}

void drawMenu() {
  display.clearDisplay();
  display.drawLine(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, SSD1306_WHITE);
  display.drawLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, 0, SSD1306_WHITE);
  for (int item = 0; item < MENU_ITEM_COUNT; item++) {
    drawMenuItem(item);
  }

  display.drawCircle(MENU_CENTER_X, MENU_CENTER_Y, 11, SSD1306_WHITE);
  display.fillCircle(static_cast<int>(menuBallX), static_cast<int>(menuBallY), 4,
                     SSD1306_WHITE);
}

void drawStatBar(int y, const char *label, int value) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, y);
  display.print(label);
  display.drawRect(48, y, 57, 8, SSD1306_WHITE);
  display.fillRect(49, y + 1, map(value, 0, 100, 0, 55), 6, SSD1306_WHITE);
  display.setCursor(109, y);
  display.print(value);
}

void drawStats() {
  display.clearDisplay();
  drawStatBar(5, "JOY", pet.joy);
  drawStatBar(20, "ENERGY", pet.energy);
  drawStatBar(35, "FULL", pet.fullness);

  display.setCursor(4, 53);
  if (!isnan(temperatureC)) {
    display.print("TEMP ");
    display.print(static_cast<int>(temperatureC));
    display.print("C");
  } else {
    display.print("TEMP --");
  }

  display.setCursor(76, 53);
  if (!isnan(humidity)) {
    display.print("H ");
    display.print(static_cast<int>(humidity));
    display.print("%");
  } else {
    display.print("H --");
  }
}

void printCentered(const char *text, int y) {
  const int width = strlen(text) * 6;
  display.setCursor((SCREEN_WIDTH - width) / 2, y);
  display.print(text);
}

void drawFood(int x, int y) {
  display.fillCircle(x, y, 3, SSD1306_WHITE);
  display.drawFastVLine(x, y - 5, 2, SSD1306_WHITE);
}

void drawBadItem(int x, int y) {
  display.drawLine(x - 3, y - 3, x + 3, y + 3, SSD1306_WHITE);
  display.drawLine(x - 3, y + 3, x + 3, y - 3, SSD1306_WHITE);
  display.drawLine(x - 2, y - 3, x + 4, y + 3, SSD1306_WHITE);
  display.drawLine(x - 2, y + 3, x + 4, y - 3, SSD1306_WHITE);
}

void drawGameOver() {
  display.fillRect(12, 8, 104, 48, SSD1306_BLACK);
  display.drawRect(12, 8, 104, 48, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  char line[24];
  printCentered("GAME OVER", 13);
  snprintf(line, sizeof(line), "SCORE %d", gameScore);
  printCentered(line, 24);
  if (gameNewBest) {
    printCentered("NEW BEST!", 34);
  } else {
    snprintf(line, sizeof(line), "BEST %d", gameBest);
    printCentered(line, 34);
  }
  printCentered("B1:RETRY B2:EXIT", 45);
}

void drawGame() {
  display.clearDisplay();

  const uint32_t now = millis();
  const int petY = SCREEN_HEIGHT - PET_SPRITE_HEIGHT;
  const int petDrawX = constrain(static_cast<int>(gamePlayerX), 0,
                                 SCREEN_WIDTH - PET_SPRITE_WIDTH);
  drawSprite(petDrawX, petY, gameFacingRight);

  for (int i = 0; i < GAME_MAX_ITEMS; i++) {
    if (!items[i].active) {
      continue;
    }
    if (items[i].bad) {
      drawBadItem(static_cast<int>(items[i].x), static_cast<int>(items[i].y));
    } else {
      drawFood(static_cast<int>(items[i].x), static_cast<int>(items[i].y));
    }
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(gameScore);
  for (int i = 0; i < gameLives; i++) {
    drawHeart(122 - i * 10, 0);
  }

  if (now < gamePopupUntil) {
    display.setCursor(petDrawX + 12, petY - 8);
    display.print(gamePopupBad ? "-1" : "+1");
  }

  if (gameOver) {
    drawGameOver();
  }
}

void drawCurrentView() {
  if (currentView == MENU_VIEW) {
    drawMenu();
  } else if (currentView == STATS_VIEW) {
    drawStats();
  } else if (currentView == GAME_VIEW) {
    drawGame();
  } else {
    drawPet();
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);
  setUpButton(buttonOne);
  setUpButton(buttonTwo);
  loadPet();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found. Check power, GND, SDA, SCL, and address.");
    while (true) {
      delay(10);
    }
  }

  mpuFound = mpu.begin(MPU6050_ADDRESS, &Wire);
  if (mpuFound) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    calibrateRestPose();
  } else {
    Serial.println("MPU6050 not found. The menu will not move until it is connected.");
  }

  if (USE_DHT11) {
    dht.begin();
    dhtFound = true;
  }

  buildFlippedSprite();
  petFacingRight = SPRITE_FACES_RIGHT;
  petDirection = SPRITE_FACES_RIGHT ? 1 : -1;
  lastFrameAt = millis();
  scheduleNextIdle(lastFrameAt);

  drawCurrentView();
}

void loop() {
  const uint32_t now = millis();
  frameSeconds = (now - lastFrameAt) / 1000.0f;
  if (frameSeconds > 0.05f) {
    frameSeconds = 0.05f;
  }
  lastFrameAt = now;

  updateMpu();
  updateDht();
  updatePetTimers();
  handleButtons();

  if (currentView == MENU_VIEW) {
    updateMenuBall();
  } else if (currentView == GAME_VIEW) {
    updateGame(now);
  } else {
    checkForShake();
  }

  updatePetMotion(now);
  updateIdle(now);

  drawCurrentView();
  delay(16);
}
