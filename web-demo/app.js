// 替换为你的真实 GitLab 项目地址，例如：https://gitlab.example.com/team/snake-robot
const GITLAB_PROJECT_URL = "https://gitlab.com/";

const authView = document.querySelector("#authView");
const dashboardView = document.querySelector("#dashboardView");
const authForm = document.querySelector("#authForm");
const realNameInput = document.querySelector("#realNameInput");
const passwordInput = document.querySelector("#passwordInput");
const roleSelect = document.querySelector("#roleSelect");
const customRoleField = document.querySelector("#customRoleField");
const customRoleInput = document.querySelector("#customRoleInput");
const toast = document.querySelector("#toast");
let telemetryTimer;
let telemetryPaused = false;

function showToast(message) {
  toast.textContent = message;
  toast.classList.add("show");
  window.clearTimeout(showToast.timer);
  showToast.timer = window.setTimeout(() => toast.classList.remove("show"), 2400);
}

function enterDashboard(name, role) {
  document.querySelector("#memberName").textContent = name;
  document.querySelector("#welcomeName").textContent = name;
  document.querySelector("#memberAvatar").textContent = Array.from(name)[0] || "员";
  document.querySelector("#memberRole").textContent = `${role} · 退出`;
  authView.classList.add("is-hidden");
  dashboardView.classList.remove("is-hidden");
  startTelemetry();
  showToast(`登录成功，欢迎 ${name}（${role}）`);
}

authForm.addEventListener("submit", (event) => {
  event.preventDefault();

  const name = realNameInput.value.trim();
  const selectedRole = roleSelect.value;
  const role = selectedRole === "其他" ? customRoleInput.value.trim() : selectedRole;

  realNameInput.value = name;
  customRoleInput.value = customRoleInput.value.trim();

  if (!authForm.checkValidity()) {
    authForm.reportValidity();
    return;
  }

  if (!name || !role) {
    showToast("请填写真实姓名并选择项目职责");
    return;
  }

  enterDashboard(name, role);
});

document.querySelector("#togglePassword").addEventListener("click", (event) => {
  const visible = passwordInput.type === "text";
  passwordInput.type = visible ? "password" : "text";
  event.currentTarget.textContent = visible ? "查看" : "隐藏";
});

roleSelect.addEventListener("change", () => {
  const usingCustomRole = roleSelect.value === "其他";
  customRoleField.classList.toggle("is-hidden", !usingCustomRole);
  customRoleInput.required = usingCustomRole;
  if (usingCustomRole) customRoleInput.focus();
});

const pageNames = {
  overview: "项目总览",
  progress: "项目进展",
  tracking: "上位机追踪",
  gitlab: "GitLab 开发",
  members: "成员管理",
};

function switchPage(page) {
  document.querySelectorAll("[data-page-content]").forEach((item) => item.classList.toggle("active", item.dataset.pageContent === page));
  document.querySelectorAll(".nav-item").forEach((item) => item.classList.toggle("active", item.dataset.page === page));
  document.querySelector("#breadcrumbText").textContent = pageNames[page] || "项目总览";
  document.querySelector(".sidebar").classList.remove("open");
  window.scrollTo({ top: 0, behavior: "smooth" });
}

document.querySelectorAll("[data-page]").forEach((item) => item.addEventListener("click", (event) => {
  event.preventDefault();
  switchPage(item.dataset.page);
}));
document.querySelectorAll("[data-go]").forEach((item) => item.addEventListener("click", () => switchPage(item.dataset.go)));
document.querySelector("#menuButton").addEventListener("click", () => document.querySelector(".sidebar").classList.toggle("open"));
document.querySelector("#logoutButton").addEventListener("click", () => {
  dashboardView.classList.add("is-hidden");
  authView.classList.remove("is-hidden");
  window.clearInterval(telemetryTimer);
  passwordInput.value = "";
  passwordInput.type = "password";
  document.querySelector("#togglePassword").textContent = "查看";
  showToast("已退出工作台");
  realNameInput.focus();
});

document.querySelector("#openGitlab").addEventListener("click", () => window.open(GITLAB_PROJECT_URL, "_blank", "noopener,noreferrer"));
document.querySelectorAll(".repo-list button").forEach((button) => button.addEventListener("click", () => window.open(GITLAB_PROJECT_URL, "_blank", "noopener,noreferrer")));

const jointBars = document.querySelector("#jointBars");
const baseJointAngles = [28, 46, 67, 34, 75, 52, 61, 41, 70, 56, 32, 48];
baseJointAngles.forEach((angle, index) => {
  const element = document.createElement("div");
  element.className = "joint-bar";
  element.innerHTML = `<i style="height:${angle}%"></i><b>${angle - 45}°</b><span>J${String(index + 1).padStart(2, "0")}</span>`;
  jointBars.appendChild(element);
});

function updateTelemetry() {
  if (telemetryPaused) return;
  const speed = (0.38 + Math.random() * 0.09).toFixed(2);
  const latency = Math.floor(21 + Math.random() * 8);
  const temperature = (45.6 + Math.random() * 1.6).toFixed(1);
  document.querySelector("#speedValue").textContent = `${speed} m/s`;
  document.querySelector("#detailSpeed").textContent = `${speed} m/s`;
  document.querySelector("#latencyValue").textContent = `${latency} ms`;
  document.querySelector("#detailLatency").textContent = `${latency} ms`;
  document.querySelector("#temperatureValue").textContent = `${temperature} °C`;
  document.querySelectorAll(".joint-bar").forEach((bar, index) => {
    const value = Math.max(10, Math.min(90, baseJointAngles[index] + Math.round((Math.random() - .5) * 12)));
    bar.querySelector("i").style.height = `${value}%`;
    bar.querySelector("b").textContent = `${value - 45}°`;
  });
}

function startTelemetry() {
  window.clearInterval(telemetryTimer);
  telemetryTimer = window.setInterval(updateTelemetry, 1400);
}

document.querySelector("#pauseTelemetry").addEventListener("click", (event) => {
  telemetryPaused = !telemetryPaused;
  event.currentTarget.textContent = telemetryPaused ? "继续数据流" : "暂停数据流";
  showToast(telemetryPaused ? "实时数据已暂停" : "实时数据已恢复");
});

const now = new Date();
document.querySelector("#todayText").textContent = new Intl.DateTimeFormat("zh-CN", { month: "long", day: "numeric", weekday: "short" }).format(now);

document.querySelectorAll(".new-task-button, .text-button").forEach((button) => button.addEventListener("click", () => showToast("该操作将在连接后端后开放")));
