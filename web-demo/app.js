const GITLAB_URL = "http://gitlab.xmutros2snake.com";
const ACCOUNT_STORAGE_KEY = "ling-snake-accounts-v1";

const authView = document.querySelector("#authView");
const dashboardView = document.querySelector("#dashboardView");
const authForm = document.querySelector("#authForm");
const realNameInput = document.querySelector("#realNameInput");
const passwordInput = document.querySelector("#passwordInput");
const roleSelect = document.querySelector("#roleSelect");
const customRoleField = document.querySelector("#customRoleField");
const customRoleInput = document.querySelector("#customRoleInput");
const authTitle = document.querySelector("#authTitle");
const authDescription = document.querySelector("#authDescription");
const passwordLabel = document.querySelector("#passwordLabel");
const authSubmit = document.querySelector("#authSubmit");
const authSwitchPrompt = document.querySelector("#authSwitchPrompt");
const switchAuthMode = document.querySelector("#switchAuthMode");
const toast = document.querySelector("#toast");
let authMode = "register";

document.querySelector("#gitlabLink").href = GITLAB_URL;

function showToast(message) {
  toast.textContent = message;
  toast.classList.add("show");
  window.clearTimeout(showToast.timer);
  showToast.timer = window.setTimeout(() => toast.classList.remove("show"), 2400);
}

function readAccounts() {
  try {
    const accounts = JSON.parse(localStorage.getItem(ACCOUNT_STORAGE_KEY) || "[]");
    return Array.isArray(accounts) ? accounts : [];
  } catch {
    return [];
  }
}

function fallbackHash(value) {
  let hash = 2166136261;
  for (const character of value) {
    hash ^= character.codePointAt(0);
    hash = Math.imul(hash, 16777619);
  }
  return `local-${(hash >>> 0).toString(16)}`;
}

async function hashPassword(password) {
  if (!globalThis.crypto?.subtle) return fallbackHash(password);
  const bytes = new TextEncoder().encode(password);
  const digest = await globalThis.crypto.subtle.digest("SHA-256", bytes);
  return Array.from(new Uint8Array(digest), (byte) => byte.toString(16).padStart(2, "0")).join("");
}

function enterDashboard(account) {
  const { name, role } = account;
  document.querySelector("#memberName").textContent = name;
  document.querySelector("#welcomeName").textContent = name;
  document.querySelector("#memberAvatar").textContent = Array.from(name)[0] || "员";
  document.querySelector("#memberRole").textContent = `${role} · 退出`;
  authView.classList.add("is-hidden");
  dashboardView.classList.remove("is-hidden");
  showToast(`欢迎回来，${name}`);
}

function setAuthMode(mode) {
  authMode = mode;
  const isRegister = mode === "register";
  document.querySelectorAll(".registration-only").forEach((field) => field.classList.toggle("is-hidden", !isRegister));
  roleSelect.required = isRegister;
  customRoleInput.required = isRegister && roleSelect.value === "其他";
  passwordInput.autocomplete = isRegister ? "new-password" : "current-password";
  authTitle.textContent = isRegister ? "注册项目账户" : "登录项目工作台";
  authDescription.textContent = isRegister ? "首次使用请注册；注册完成后将直接进入工作台。" : "请输入已注册的姓名和密码。";
  passwordLabel.textContent = isRegister ? "设置密码" : "登录密码";
  authSubmit.querySelector("span").textContent = isRegister ? "注册并进入工作台" : "登录工作台";
  authSwitchPrompt.textContent = isRegister ? "已有账户？" : "还没有账户？";
  switchAuthMode.textContent = isRegister ? "直接登录" : "立即注册";
  passwordInput.value = "";
  customRoleField.classList.toggle("is-hidden", !isRegister || roleSelect.value !== "其他");
}

authForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const name = realNameInput.value.trim();
  const password = passwordInput.value;
  const selectedRole = roleSelect.value;
  const role = selectedRole === "其他" ? customRoleInput.value.trim() : selectedRole;

  realNameInput.value = name;
  customRoleInput.value = customRoleInput.value.trim();
  if (!authForm.checkValidity()) {
    authForm.reportValidity();
    return;
  }

  const accounts = readAccounts();
  const passwordHash = await hashPassword(password);
  if (authMode === "register") {
    if (!name || !role) {
      showToast("请填写真实姓名并选择项目职责");
      return;
    }
    if (accounts.some((account) => account.name === name)) {
      showToast("该姓名已注册，请直接登录");
      setAuthMode("login");
      return;
    }
    const account = { name, role, passwordHash };
    localStorage.setItem(ACCOUNT_STORAGE_KEY, JSON.stringify([...accounts, account]));
    enterDashboard(account);
    return;
  }

  const account = accounts.find((item) => item.name === name && item.passwordHash === passwordHash);
  if (!account) {
    showToast("姓名或密码不正确；首次使用请先注册");
    return;
  }
  enterDashboard(account);
});

switchAuthMode.addEventListener("click", () => setAuthMode(authMode === "register" ? "login" : "register"));

document.querySelector("#togglePassword").addEventListener("click", (event) => {
  const visible = passwordInput.type === "text";
  passwordInput.type = visible ? "password" : "text";
  event.currentTarget.textContent = visible ? "查看" : "隐藏";
});

roleSelect.addEventListener("change", () => {
  const usingCustomRole = authMode === "register" && roleSelect.value === "其他";
  customRoleField.classList.toggle("is-hidden", !usingCustomRole);
  customRoleInput.required = usingCustomRole;
  if (usingCustomRole) customRoleInput.focus();
});

const pageNames = {
  overview: "项目总览",
  progress: "项目进展",
  members: "成员管理",
};

function switchPage(page) {
  document.querySelectorAll("[data-page-content]").forEach((item) => item.classList.toggle("active", item.dataset.pageContent === page));
  document.querySelectorAll(".nav-item[data-page]").forEach((item) => item.classList.toggle("active", item.dataset.page === page));
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
  passwordInput.value = "";
  passwordInput.type = "password";
  document.querySelector("#togglePassword").textContent = "查看";
  setAuthMode("login");
  showToast("已退出工作台");
  realNameInput.focus();
});

const now = new Date();
document.querySelector("#todayText").textContent = new Intl.DateTimeFormat("zh-CN", { month: "long", day: "numeric", weekday: "short" }).format(now);

document.querySelectorAll(".new-task-button, .text-button").forEach((button) => button.addEventListener("click", () => showToast("该操作将在连接后端后开放")));
