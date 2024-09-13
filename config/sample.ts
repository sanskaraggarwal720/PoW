import fs from "fs";

interface FirewallConfig {
  blockedIPs: string[];
  logFile: string;
}

class FirewallManager {
  config: FirewallConfig;

  constructor(configFile: string) {
    this.config = JSON.parse(fs.readFileSync(configFile, "utf-8"));
  }

  blockIP(ip: string) {
    if (!this.config.blockedIPs.includes(ip)) {
      this.config.blockedIPs.push(ip);
      this.saveConfig();
      console.log(`IP ${ip} blocked`);
    }
  }

  unblockIP(ip: string) {
    const index = this.config.blockedIPs.indexOf(ip);
    if (index !== -1) {
      this.config.blockedIPs.splice(index, 1);
      this.saveConfig();
      console.log(`IP ${ip} unblocked`);
    }
  }

  saveConfig() {
    fs.writeFileSync(
      "./firewall_config.json",
      JSON.stringify(this.config, null, 2),
    );
  }

  logBlockedIP(ip: string) {
    const logEntry = `${new Date().toISOString()} - Blocked IP: ${ip}\n`;
    fs.appendFileSync(this.config.logFile, logEntry);
  }
}

// Usage example:
const firewallManager = new FirewallManager("./firewall_config.json");
firewallManager.blockIP("192.168.0.1");
firewallManager.unblockIP("192.168.0.1");