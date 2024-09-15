document.addEventListener("DOMContentLoaded", function () {
  const data = {
    "10.0.0.1": {
      Details:
        '{"Browser":"Firefox","Request Count":"42","Response Time (ms)":"720","DDoS Happening":"NoDDoS","Status Code":"403","HTTP Method":"OPTIONS","Timestamp":"17251488473N"}',
    },
    "127.0.0.1": {
      Details:
        '{"Status Code":"403","Response Time (ms)":"825","Request Count":"79","DDoS Happening":"NoDDoS","Browser":"Chrome","Timestamp":"17251488473N","HTTP Method":"POST"}',
    },
    "172.16.0.1": {
      Details:
        '{"HTTP Method":"HEAD","Request Count":"65","Status Code":"301","Response Time (ms)":"510","Browser":"Opera","DDoS Happening":"NoDDoS","Timestamp":"17251488473N"}',
    },
    "192.168.0.1": {
      Details:
        '{"Response Time (ms)":"393","DDoS Happening":"NoDDoS","Request Count":"47","Browser":"Edge","HTTP Method":"GET","Timestamp":"17251488473N","Status Code":"502"}',
    },
    "198.51.100.0": {
      Details:
        '{"HTTP Method":"POST","Timestamp":"17251488523N","Request Count":"309","Response Time (ms)":"2240","Browser":"Firefox","DDoS Happening":"DDOS","Status Code":"502"}',
    },
    "203.0.113.0": {
      Details:
        '{"Timestamp":"17251488523N","HTTP Method":"GET","Request Count":"67","Browser":"NOT-FOUND","Status Code":"404","Response Time (ms)":"713","DDoS Happening":"NoDDoS"}',
    },

    // Add more IPs here...
  };

  const tbody = document.querySelector("tbody");

  for (const ip in data) {
    if (data.hasOwnProperty(ip)) {
      const details = JSON.parse(data[ip].Details);
      const row = document.createElement("tr");

      row.innerHTML = `
                <td>${ip}</td>
                <td>${details.Timestamp}</td>
                <td>${details.Browser}</td>
                <td>${details["Request Count"]}</td>
                <td>${details["HTTP Method"]}</td>
                <td>${details["Status Code"]}</td>
                <td>${details["Response Time (ms)"]}</td>
                <td>${details["DDoS Happening"]}</td>
            `;

      tbody.appendChild(row);
    }
  }
});
