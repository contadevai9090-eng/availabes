const config = require('../config.json');

const cooldowns = new Map();

function isOnCooldown(userId) {
  const now = Date.now();
  const expiry = cooldowns.get(userId);
  if (expiry && now < expiry) {
    const remaining = Math.ceil((expiry - now) / 1000);
    return remaining;
  }
  return 0;
}

function setCooldown(userId) {
  cooldowns.set(userId, Date.now() + config.cooldownSeconds * 1000);
}

module.exports = { isOnCooldown, setCooldown };
